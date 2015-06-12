#include "mbed.h"
#include "Modem.h"
#include "APN.h"

#define PROFILE         "0"   //!< this is the psd profile used
#define MAX_SIZE        128   //!< max expected messages
// num sockets
#define NUMSOCKETS      (sizeof(_sockets)/sizeof(*_sockets))
//! test if it is a socket is ok to use
#define ISSOCKET(s)     (((s) >= 0) && ((s) < NUMSOCKETS) && (_sockets[s].handle != SOCKET_ERROR))

//! check for timeout
#define TIMEOUT(t, ms)  ((ms != TIMEOUT_BLOCKING) && (ms < t.read_ms()))
//! registration ok check helper
#define REG_OK(r)       ((r == REG_HOME) || (r == REG_ROAMING))
//! registration done check helper (no need to poll further)
#define REG_DONE(r)     ((r == REG_HOME) || (r == REG_ROAMING) || (r == REG_DENIED))

#define ERROR(...) (void)0 // no tracing
#define TEST(...)  (void)0 // no tracing
#define INFO(...)  (void)0 // no tracing
#define TRACE(...) (void)0 // no tracing

MDMParser* MDMParser::inst;

MDMParser::MDMParser(void)
{
    inst = this;
    memset(&_dev, 0, sizeof(_dev));
    memset(&_net, 0, sizeof(_net));
    _net.lac   = 0xFFFF;
    _net.ci    = 0xFFFFFFFF;
    _ip        = NOIP;
    _init      = false;
    memset(_sockets, 0, sizeof(_sockets));
    for (int socket = 0; socket < NUMSOCKETS; socket++)
        _sockets[socket].handle = SOCKET_ERROR;
}

int MDMParser::send(const char* buf, int len)
{
    return _send(buf, len);
}

int MDMParser::sendFormated(const char* format, ...) {
    char buf[MAX_SIZE];
    va_list args;
    va_start(args, format);
    int len = vsnprintf(buf, sizeof(buf), format, args);
    va_end(args);
    return send(buf, len);
}

int MDMParser::waitFinalResp(_CALLBACKPTR cb /* = NULL*/,
                             void* param /* = NULL*/,
                             int timeout_ms /*= 5000*/)
{
    char buf[MAX_SIZE + 64 /* add some more space for framing */];
    Timer timer;
    timer.start();
    do {
        int ret = getLine(buf, sizeof(buf));
        if ((ret != WAIT) && (ret != NOT_FOUND))
        {
            int type = TYPE(ret);
            // handle unsolicited commands here
            if (type == TYPE_PLUS) {
                const char* cmd = buf+3;
                int a, b, c, d, r;
                char s[32];

                // SMS Command ---------------------------------
                // +CNMI: <mem>,<index>
                if (sscanf(cmd, "CMTI: \"%*[^\"]\",%d", &a) == 1) {
                    TRACE("New SMS at index %d\r\n", a);
                // Socket Specific Command ---------------------------------
                // +UUSORD: <socket>,<length>
                } else if ((sscanf(cmd, "UUSORD: %d,%d", &a, &b) == 2)) {
                    int socket = _findSocket(a);
                    TRACE("Socket %d: handle %d has %d bytes pending\r\n", socket, a, b);
                    if (socket != SOCKET_ERROR)
                        _sockets[socket].pending = b;
                // +UUSORF: <socket>,<length>
                } else if ((sscanf(cmd, "UUSORF: %d,%d", &a, &b) == 2)) {
                    int socket = _findSocket(a);
                    TRACE("Socket %d: handle %d has %d bytes pending\r\n", socket, a, b);
                    if (socket != SOCKET_ERROR)
                        _sockets[socket].pending = b;
                // +UUSOCL: <socket>
                } else if ((sscanf(cmd, "UUSOCL: %d", &a) == 1)) {
                    int socket = _findSocket(a);
                    TRACE("Socket %d: handle %d closed by remote host\r\n", socket, a);
                    if ((socket != SOCKET_ERROR) && _sockets[socket].connected)
                        _sockets[socket].connected = false;
                }
                // +UUPSDD: <profile_id>
		if (sscanf(cmd, "UUPSDD: %d",&a) == 1) {
		    if (*PROFILE == a) _ip = NOIP;
		} else {
		    // +CREG|CGREG: <n>,<stat>[,<lac>,<ci>[,AcT[,<rac>]]] // reply to AT+CREG|AT+CGREG
		    // +CREG|CGREG: <stat>[,<lac>,<ci>[,AcT[,<rac>]]]     // URC
		    b = 0xFFFF; c = 0xFFFFFFFF; d = -1;
		    r = sscanf(cmd, "%s %*d,%d,\"%X\",\"%X\",%d",s,&a,&b,&c,&d);
		    if (r <= 1)
			r = sscanf(cmd, "%s %d,\"%X\",\"%X\",%d",s,&a,&b,&c,&d);
		    if (r >= 2) {
			Reg *reg = !strcmp(s, "CREG:")  ? &_net.csd :
				   !strcmp(s, "CGREG:") ? &_net.psd : NULL;
			if (reg) {
			    // network status
			    if      (a == 0) *reg = REG_NONE;     // 0: not registered, home network
			    else if (a == 1) *reg = REG_HOME;     // 1: registered, home network
			    else if (a == 2) *reg = REG_NONE;     // 2: not registered, but MT is currently searching a new operator to register to
			    else if (a == 3) *reg = REG_DENIED;   // 3: registration denied
			    else if (a == 4) *reg = REG_UNKNOWN;  // 4: unknown
			    else if (a == 5) *reg = REG_ROAMING;  // 5: registered, roaming
			    if ((r >= 3) && (b != 0xFFFF))                _net.lac = b; // location area code
			    if ((r >= 4) && ((unsigned)c != 0xFFFFFFFF))  _net.ci  = c; // cell ID
			    // access technology
			    if (r >= 5) {
				if      (d == 0) _net.act = ACT_GSM;      // 0: GSM
				else if (d == 1) _net.act = ACT_GSM;      // 1: GSM COMPACT
				else if (d == 2) _net.act = ACT_UTRAN;    // 2: UTRAN
				else if (d == 3) _net.act = ACT_EDGE;     // 3: GSM with EDGE availability
				else if (d == 4) _net.act = ACT_UTRAN;    // 4: UTRAN with HSDPA availability
				else if (d == 5) _net.act = ACT_UTRAN;    // 5: UTRAN with HSUPA availability
				else if (d == 6) _net.act = ACT_UTRAN;    // 6: UTRAN with HSDPA and HSUPA availability
			    }
			}
		    }
		}
            }
            if (cb) {
                int len = LENGTH(ret);
                int ret = cb(type, buf, len, param);
                if (WAIT != ret)
                    return ret;
            }
            if (type == TYPE_OK)
                return RESP_OK;
            if (type == TYPE_ERROR)
                return RESP_ERROR;
            if (type == TYPE_PROMPT)
                return RESP_PROMPT;
        }
        // relax a bit
        wait_ms(10);
    }
    while (!TIMEOUT(timer, timeout_ms));
    return WAIT;
}

int MDMParser::_cbString(int type, const char* buf, int len, char* str)
{
    if (str && (type == TYPE_UNKNOWN)) {
        if (sscanf(buf, "\r\n%s\r\n", str) == 1)
            /*nothing*/;
    }
    return WAIT;
}

int MDMParser::_cbInt(int type, const char* buf, int len, int* val)
{
    if (val && (type == TYPE_UNKNOWN)) {
        if (sscanf(buf, "\r\n%d\r\n", val) == 1)
            /*nothing*/;
    }
    return WAIT;
}

// ----------------------------------------------------------------

bool MDMParser::connect(
            const char* simpin,
            const char* apn, const char* username,
            const char* password, Auth auth,
            PinName pn)
{
    bool ok = init(simpin, NULL, pn);
#ifdef MDM_DEBUG
    dumpDevStatus(&_dev);
#endif
    if (!ok)
        return false;
    ok = registerNet();
#ifdef MDM_DEBUG
    dumpNetStatus(&_net);
#endif
    if (!ok)
        return false;
    IP ip = join(apn, username, password, auth);
#ifdef MDM_DEBUG
    dumpIp(ip);
#endif
    if (ip == NOIP)
        return false;
    return true;
}

bool MDMParser::init(const char* simpin, DevStatus* status, PinName pn)
{
    int i = 10;
    memset(&_dev, 0, sizeof(_dev));
    if (pn != NC) {
        DigitalOut pin(pn, 1);
        while (i--) {
            // SARA-U270 50..80us
            pin = 0; ::wait_us(50);
            pin = 1; ::wait_ms(10);

            // purge any messages
            purge();

            // check interface
            sendFormated("AT\r\n");
            int r = waitFinalResp(NULL, NULL, 1000);
            if(RESP_OK == r) break;
        }
        if (i < 0) {
            ERROR("No Reply from Modem\r\n");
            goto failure;
        }
    }
    _init = true;

    // echo off
    sendFormated("AT E0\r\n");
    if(RESP_OK != waitFinalResp())
        goto failure;
    // enable verbose error messages
    sendFormated("AT+CMEE=2\r\n");
    if(RESP_OK != waitFinalResp())
        goto failure;
    // set baud rate
    sendFormated("AT+IPR=115200\r\n");
    if (RESP_OK != waitFinalResp())
        goto failure;
    // wait some time until baudrate is applied
    wait_ms(200); // SARA-G > 40ms
    // identify the module
    sendFormated("ATI\r\n");
    if (RESP_OK != waitFinalResp(_cbATI, &_dev.dev))
        goto failure;
    if (_dev.dev == DEV_UNKNOWN)
        goto failure;
    // device specific init
    // enable the network identification feature
    sendFormated("AT+UGPIOC=16,2\r\n");
    if (RESP_OK != waitFinalResp())
	goto failure;
    // check the sim card
    for (int i = 0; (i < 5) && (_dev.sim != SIM_READY); i++) {
	sendFormated("AT+CPIN?\r\n");
	int ret = waitFinalResp(_cbCPIN, &_dev.sim);
	// having an error here is ok (sim may still be initialising)
	if ((RESP_OK != ret) && (RESP_ERROR != ret))
	    goto failure;
	// Enter PIN if needed
	if (_dev.sim == SIM_PIN) {
	    if (!simpin) {
		ERROR("SIM PIN not available\r\n");
		goto failure;
	    }
	    sendFormated("AT+CPIN=%s\r\n", simpin);
	    if (RESP_OK != waitFinalResp(_cbCPIN, &_dev.sim))
		goto failure;
	} else if (_dev.sim != SIM_READY) {
	    wait_ms(1000);
	}
    }
    if (_dev.sim != SIM_READY) {
	if (_dev.sim == SIM_MISSING)
	    ERROR("SIM not inserted\r\n");
	goto failure;
    }
    // get the manufacturer
    sendFormated("AT+CGMI\r\n");
    if (RESP_OK != waitFinalResp(_cbString, _dev.manu))
	goto failure;
    // get the model identification
    sendFormated("AT+CGMM\r\n");
    if (RESP_OK != waitFinalResp(_cbString, _dev.model))
	goto failure;
    // get the modem firmware version
    sendFormated("AT+CGMR\r\n");
    if (RESP_OK != waitFinalResp(_cbString, _dev.ver))
	goto failure;
    // Returns the ICCID (Integrated Circuit Card ID) of the SIM-card.
    // ICCID is a serial number identifying the SIM.
    sendFormated("AT+CCID\r\n");
    if (RESP_OK != waitFinalResp(_cbCCID, _dev.ccid))
	goto failure;
    // Returns the product serial number, IMEI (International Mobile Equipment Identity)
    sendFormated("AT+CGSN\r\n");
    if (RESP_OK != waitFinalResp(_cbString, _dev.imei))
	goto failure;
    // enable power saving
    if (_dev.lpm != LPM_DISABLED) {
	 // enable power saving (requires flow control, cts at least)
	sendFormated("AT+UPSV=1\r\n");
	if (RESP_OK != waitFinalResp())
	    goto failure;
	_dev.lpm = LPM_ACTIVE;
    }
    // enable the psd registration unsolicited result code
    sendFormated("AT+CGREG=2\r\n");
    if (RESP_OK != waitFinalResp())
	goto failure;
    // enable the network registration unsolicited result code
    sendFormated("AT+CREG=%d\r\n", 2);
    if (RESP_OK != waitFinalResp())
        goto failure;
    // Setup SMS in text mode
    sendFormated("AT+CMGF=1\r\n");
    if (RESP_OK != waitFinalResp())
        goto failure;
    // setup new message indication
    sendFormated("AT+CNMI=2,1\r\n");
    if (RESP_OK != waitFinalResp())
        goto failure;
    // Request IMSI (International Mobile Subscriber Identification)
    sendFormated("AT+CIMI\r\n");
    if (RESP_OK != waitFinalResp(_cbString, _dev.imsi))
        goto failure;
    if (status)
        memcpy(status, &_dev, sizeof(DevStatus));
    return true;
failure:
    return false;
}

bool MDMParser::powerOff(void)
{
    bool ok = false;
    if (_init) {
        INFO("Modem::powerOff\r\n");
        sendFormated("AT+CPWROFF\r\n");
        if (RESP_OK == waitFinalResp(NULL, NULL, 120*1000)) {
            _init = false;
            ok = true;
        }
    }
    return ok;
}

int MDMParser::_cbATI(int type, const char* buf, int len, Dev* dev)
{
    if ((type == TYPE_UNKNOWN) && dev) {
        if      (strstr(buf, "SARA-G350")) *dev = DEV_SARA_G350;
        else if (strstr(buf, "LISA-U200")) *dev = DEV_LISA_U200;
        else if (strstr(buf, "LISA-C200")) *dev = DEV_LISA_C200;
        else if (strstr(buf, "SARA-U260")) *dev = DEV_SARA_U260;
        else if (strstr(buf, "SARA-U270")) *dev = DEV_SARA_U270;
        else if (strstr(buf, "LEON-G200")) *dev = DEV_LEON_G200;
    }
    return WAIT;
}

int MDMParser::_cbCPIN(int type, const char* buf, int len, Sim* sim)
{
    if (sim) {
        if (type == TYPE_PLUS){
            char s[16];
            if (sscanf(buf, "\r\n+CPIN: %[^\r]\r\n", s) >= 1)
                *sim = (0 == strcmp("READY", s)) ? SIM_READY : SIM_PIN;
        } else if (type == TYPE_ERROR) {
            if (strstr(buf, "+CME ERROR: SIM not inserted"))
                *sim = SIM_MISSING;
        }
    }
    return WAIT;
}

int MDMParser::_cbCCID(int type, const char* buf, int len, char* ccid)
{
    if ((type == TYPE_PLUS) && ccid){
        if (sscanf(buf, "\r\n+CCID: %[^\r]\r\n", ccid) == 1)
            /*TRACE("Got CCID: %s\r\n", ccid)*/;
    }
    return WAIT;
}

bool MDMParser::registerNet(NetStatus* status /*= NULL*/, int timeout_ms /*= 180000*/)
{
    Timer timer;
    timer.start();
    INFO("Modem::register\r\n");
    while (!checkNetStatus(status) && !TIMEOUT(timer, timeout_ms))
        wait_ms(1000);
    if (_net.csd == REG_DENIED) ERROR("CSD Registration Denied\r\n");
    if (_net.psd == REG_DENIED) ERROR("PSD Registration Denied\r\n");
    return REG_OK(_net.csd) || REG_OK(_net.psd);
}

bool MDMParser::checkNetStatus(NetStatus* status /*= NULL*/)
{
    bool ok = false;
    memset(&_net, 0, sizeof(_net));
    _net.lac = 0xFFFF;
    _net.ci = 0xFFFFFFFF;
    // check registration
    sendFormated("AT+CREG?\r\n");
    waitFinalResp();     // don't fail as service could be not subscribed
    // check PSD registration
    sendFormated("AT+CGREG?\r\n");
    waitFinalResp(); // don't fail as service could be not subscribed

    if (REG_OK(_net.csd) || REG_OK(_net.psd))
    {
        // check modem specific status messages
        sendFormated("AT+COPS?\r\n");
        if (RESP_OK != waitFinalResp(_cbCOPS, &_net))
	    goto failure;
        // get the MSISDNs related to this subscriber
        sendFormated("AT+CNUM\r\n");
        if (RESP_OK != waitFinalResp(_cbCNUM, _net.num))
            goto failure;
        // get the signal strength indication
        sendFormated("AT+CSQ\r\n");
        if (RESP_OK != waitFinalResp(_cbCSQ, &_net))
            goto failure;
    }
    if (status) {
        memcpy(status, &_net, sizeof(NetStatus));
    }
    ok = REG_DONE(_net.csd) && REG_DONE(_net.psd);
    return ok;
failure:
    return false;
}

int MDMParser::_cbCOPS(int type, const char* buf, int len, NetStatus* status)
{
    if ((type == TYPE_PLUS) && status){
        int act = 99;
        // +COPS: <mode>[,<format>,<oper>[,<AcT>]]
        if (sscanf(buf, "\r\n+COPS: %*d,%*d,\"%[^\"]\",%d",status->opr,&act) >= 1) {
            if      (act == 0) status->act = ACT_GSM;      // 0: GSM,
            else if (act == 2) status->act = ACT_UTRAN;    // 2: UTRAN
        }
    }
    return WAIT;
}

int MDMParser::_cbCNUM(int type, const char* buf, int len, char* num)
{
    if ((type == TYPE_PLUS) && num){
        int a;
        if ((sscanf(buf, "\r\n+CNUM: \"My Number\",\"%31[^\"]\",%d", num, &a) == 2) &&
            ((a == 129) || (a == 145))) {
        }
    }
    return WAIT;
}

int MDMParser::_cbCSQ(int type, const char* buf, int len, NetStatus* status)
{
    if ((type == TYPE_PLUS) && status){
        int a,b;
        char _ber[] = { 49, 43, 37, 25, 19, 13, 7, 0 }; // see 3GPP TS 45.008 [20] subclause 8.2.4
        // +CSQ: <rssi>,<qual>
        if (sscanf(buf, "\r\n+CSQ: %d,%d",&a,&b) == 2) {
            if (a != 99) status->rssi = -113 + 2*a;  // 0: -113 1: -111 ... 30: -53 dBm with 2 dBm steps
            if ((b != 99) && (b < (int)sizeof(_ber))) status->ber = _ber[b];  //
        }
    }
    return WAIT;
}

int MDMParser::_cbUACTIND(int type, const char* buf, int len, int* i)
{
    if ((type == TYPE_PLUS) && i){
        int a;
        if (sscanf(buf, "\r\n+UACTIND: %d", &a) == 1) {
            *i = a;
        }
    }
    return WAIT;
}

// ----------------------------------------------------------------
// internet connection

MDMParser::IP MDMParser::join(const char* apn /*= NULL*/, const char* username /*= NULL*/,
                              const char* password /*= NULL*/, Auth auth /*= AUTH_DETECT*/)
{
    INFO("Modem::join\r\n");
    int a = 0;
    bool force = true;
    _ip = NOIP;
    // check gprs attach status
    sendFormated("AT+CGATT=1\r\n");
    if (RESP_OK != waitFinalResp(NULL, NULL, 3*60*1000))
	goto failure;

    // Check the profile
    sendFormated("AT+UPSND=" PROFILE ",8\r\n");
    if (RESP_OK != waitFinalResp(_cbUPSND, &a))
	goto failure;
    if (a == 1 && force) {
	// disconnect the profile already if it is connected
	sendFormated("AT+UPSDA=" PROFILE ",4\r\n");
	if (RESP_OK != waitFinalResp(NULL, NULL, 40*1000))
	    goto failure;
	a = 0;
    }
    if (a == 0) {
	bool ok = false;
	// try to lookup the apn settings from our local database by mccmnc
	const char* config = NULL;
	if (!apn && !username && !password)
	    config = apnconfig(_dev.imsi);

	// Set up the dynamic IP address assignment.
	sendFormated("AT+UPSD=" PROFILE ",7,\"0.0.0.0\"\r\n");
	if (RESP_OK != waitFinalResp())
	    goto failure;

	do {
	    if (config) {
		apn      = _APN_GET(config);
		username = _APN_GET(config);
		password = _APN_GET(config);
		TRACE("Testing APN Settings(\"%s\",\"%s\",\"%s\")\r\n", apn, username, password);
	    }
	    // Set up the APN
	    if (apn && *apn) {
		sendFormated("AT+UPSD=" PROFILE ",1,\"%s\"\r\n", apn);
		if (RESP_OK != waitFinalResp())
		    goto failure;
	    }
	    if (username && *username) {
		sendFormated("AT+UPSD=" PROFILE ",2,\"%s\"\r\n", username);
		if (RESP_OK != waitFinalResp())
		    goto failure;
	    }
	    if (password && *password) {
		sendFormated("AT+UPSD=" PROFILE ",3,\"%s\"\r\n", password);
		if (RESP_OK != waitFinalResp())
		    goto failure;
	    }
	    // try different Authentication Protocols
	    // 0 = none
	    // 1 = PAP (Password Authentication Protocol)
	    // 2 = CHAP (Challenge Handshake Authentication Protocol)
	    for (int i = AUTH_NONE; i <= AUTH_CHAP && !ok; i ++) {
		if ((auth == AUTH_DETECT) || (auth == i)) {
		    // Set up the Authentication Protocol
		    sendFormated("AT+UPSD=" PROFILE ",6,%d\r\n", i);
		    if (RESP_OK != waitFinalResp())
			goto failure;
		    // Activate the profile and make connection
		    sendFormated("AT+UPSDA=" PROFILE ",3\r\n");
		    if (RESP_OK == waitFinalResp(NULL, NULL, 150*1000))
			ok = true;
		}
	    }
	} while (!ok && config && *config); // maybe use next setting ?
	if (!ok) {
	    ERROR("Your modem APN/password/username may be wrong\r\n");
	    goto failure;
	}
    }
    //Get local IP address
    sendFormated("AT+UPSND=" PROFILE ",0\r\n");
    if (RESP_OK != waitFinalResp(_cbUPSND, &_ip))
	goto failure;
    return _ip;
failure:
    return NOIP;
}

int MDMParser::_cbUDOPN(int type, const char* buf, int len, char* mccmnc)
{
    if ((type == TYPE_PLUS) && mccmnc) {
        if (sscanf(buf, "\r\n+UDOPN: 0,\"%[^\"]\"", mccmnc) == 1)
            ;
    }
    return WAIT;
}

int MDMParser::_cbCMIP(int type, const char* buf, int len, IP* ip)
{
    if ((type == TYPE_UNKNOWN) && ip) {
        int a,b,c,d;
        if (sscanf(buf, "\r\n" IPSTR, &a,&b,&c,&d) == 4)
            *ip = IPADR(a,b,c,d);
    }
    return WAIT;
}

int MDMParser::_cbUPSND(int type, const char* buf, int len, int* act)
{
    if ((type == TYPE_PLUS) && act) {
        if (sscanf(buf, "\r\n+UPSND: %*d,%*d,%d", act) == 1)
            /*nothing*/;
    }
    return WAIT;
}

int MDMParser::_cbUPSND(int type, const char* buf, int len, IP* ip)
{
    if ((type == TYPE_PLUS) && ip) {
        int a,b,c,d;
        // +UPSND=<profile_id>,<param_tag>[,<dynamic_param_val>]
        if (sscanf(buf, "\r\n+UPSND: " PROFILE ",0,\"" IPSTR "\"", &a,&b,&c,&d) == 4)
            *ip = IPADR(a,b,c,d);
    }
    return WAIT;
}

int MDMParser::_cbUDNSRN(int type, const char* buf, int len, IP* ip)
{
    if ((type == TYPE_PLUS) && ip) {
        int a,b,c,d;
        if (sscanf(buf, "\r\n+UDNSRN: \"" IPSTR "\"", &a,&b,&c,&d) == 4)
            *ip = IPADR(a,b,c,d);
    }
    return WAIT;
}

bool MDMParser::disconnect(void)
{
    bool ok = false;
    INFO("Modem::disconnect\r\n");
    if (_ip != NOIP) {
        sendFormated("AT+UPSDA=" PROFILE ",4\r\n");
        if (RESP_OK != waitFinalResp()) {
            _ip = NOIP;
            ok = true;
	}
    }
    return ok;
}

MDMParser::IP MDMParser::gethostbyname(const char* host)
{
    IP ip = NOIP;
    int a,b,c,d;
    if (sscanf(host, IPSTR, &a,&b,&c,&d) == 4)
        ip = IPADR(a,b,c,d);
    else {
        sendFormated("AT+UDNSRN=0,\"%s\"\r\n", host);
        if (RESP_OK != waitFinalResp(_cbUDNSRN, &ip))
            ip = NOIP;
    }
    return ip;
}

// ----------------------------------------------------------------
// sockets

int MDMParser::_cbUSOCR(int type, const char* buf, int len, int* handle)
{
    if ((type == TYPE_PLUS) && handle) {
        // +USOCR: socket
        if (sscanf(buf, "\r\n+USOCR: %d", handle) == 1)
            /*nothing*/;
    }
    return WAIT;
}

int MDMParser::socketSocket(IpProtocol ipproto, int port)
{
    int socket;
    // find an free socket
    socket = _findSocket();
    TRACE("socketSocket(%d)\r\n", ipproto);
    if (socket != SOCKET_ERROR) {
        if (ipproto == IPPROTO_UDP) {
            // sending port can only be set on 2G/3G modules
            if ((port != -1) && (_dev.dev != DEV_LISA_C200)) {
                sendFormated("AT+USOCR=17,%d\r\n", port);
            } else {
                sendFormated("AT+USOCR=17\r\n");
            }
        } else /*(ipproto == IPPROTO_TCP)*/ {
            sendFormated("AT+USOCR=6\r\n");
        }
        int handle = SOCKET_ERROR;
        if ((RESP_OK == waitFinalResp(_cbUSOCR, &handle)) &&
            (handle != SOCKET_ERROR)) {
            TRACE("Socket %d: handle %d was created\r\n", socket, handle);
            _sockets[socket].handle     = handle;
            _sockets[socket].timeout_ms = TIMEOUT_BLOCKING;
            _sockets[socket].connected  = false;
            _sockets[socket].pending    = 0;
        }
        else
            socket = SOCKET_ERROR;
    }
    return socket;
}

bool MDMParser::socketConnect(int socket, const char * host, int port)
{
    IP ip = gethostbyname(host);
    if (ip == NOIP)
        return false;
    // connect to socket
    bool ok = false;
    if (ISSOCKET(socket) && (!_sockets[socket].connected)) {
        TRACE("socketConnect(%d,%s,%d)\r\n", socket,host,port);
        sendFormated("AT+USOCO=%d,\"" IPSTR "\",%d\r\n", _sockets[socket].handle, IPNUM(ip), port);
        if (RESP_OK == waitFinalResp())
            ok = _sockets[socket].connected = true;
    }
    return ok;
}

bool MDMParser::socketIsConnected(int socket)
{
    bool ok = false;
    ok = ISSOCKET(socket) && _sockets[socket].connected;
    TRACE("socketIsConnected(%d) %s\r\n", socket, ok?"yes":"no");
    return ok;
}

bool MDMParser::socketSetBlocking(int socket, int timeout_ms)
{
    bool ok = false;
    TRACE("socketSetBlocking(%d,%d)\r\n", socket,timeout_ms);
    if (ISSOCKET(socket)) {
        _sockets[socket].timeout_ms = timeout_ms;
        ok = true;
    }
    return ok;
}

bool  MDMParser::socketClose(int socket)
{
    bool ok = false;
    if (ISSOCKET(socket) && _sockets[socket].connected) {
        TRACE("socketClose(%d)\r\n", socket);
        sendFormated("AT+USOCL=%d\r\n", _sockets[socket].handle);
        if (RESP_OK == waitFinalResp()) {
            _sockets[socket].connected = false;
            ok = true;
        }
    }
    return ok;
}

bool  MDMParser::socketFree(int socket)
{
    // make sure it is closed
    socketClose(socket);
    bool ok = true;
    if (ISSOCKET(socket)) {
        TRACE("socketFree(%d)\r\n",  socket);
        _sockets[socket].handle     = SOCKET_ERROR;
        _sockets[socket].timeout_ms = TIMEOUT_BLOCKING;
        _sockets[socket].connected  = false;
        _sockets[socket].pending    = 0;
        ok = true;
    }
    return ok;
}

#define USO_MAX_WRITE 1024 //!< maximum number of bytes to write to socket

int MDMParser::socketSend(int socket, const char * buf, int len)
{
    TRACE("socketSend(%d,,%d)\r\n", socket,len);
    int cnt = len;
    while (cnt > 0) {
        int blk = USO_MAX_WRITE;
        if (cnt < blk)
            blk = cnt;
        bool ok = false;
        if (ISSOCKET(socket)) {
            sendFormated("AT+USOWR=%d,%d\r\n",_sockets[socket].handle,blk);
            if (RESP_PROMPT == waitFinalResp()) {
                wait_ms(50);
                send(buf, blk);
                if (RESP_OK == waitFinalResp())
                    ok = true;
            }
        }
        if (!ok)
            return SOCKET_ERROR;
        buf += blk;
        cnt -= blk;
    }
    return (len - cnt);
}

int MDMParser::socketSendTo(int socket, IP ip, int port, const char * buf, int len)
{
    TRACE("socketSendTo(%d," IPSTR ",%d,,%d)\r\n", socket,IPNUM(ip),port,len);
    int cnt = len;
    while (cnt > 0) {
        int blk = USO_MAX_WRITE;
        if (cnt < blk)
            blk = cnt;
        bool ok = false;
        if (ISSOCKET(socket)) {
            sendFormated("AT+USOST=%d,\"" IPSTR "\",%d,%d\r\n",_sockets[socket].handle,IPNUM(ip),port,blk);
            if (RESP_PROMPT == waitFinalResp()) {
                wait_ms(50);
                send(buf, blk);
                if (RESP_OK == waitFinalResp())
                    ok = true;
            }
        }
        if (!ok)
            return SOCKET_ERROR;
        buf += blk;
        cnt -= blk;
    }
    return (len - cnt);
}

int MDMParser::socketReadable(int socket)
{
    int pending = SOCKET_ERROR;
    if (ISSOCKET(socket) && _sockets[socket].connected) {
        TRACE("socketReadable(%d)\r\n", socket);
        // allow to receive unsolicited commands
        waitFinalResp(NULL, NULL, 0);
        if (_sockets[socket].connected)
           pending = _sockets[socket].pending;
    }
    return pending;
}

int MDMParser::_cbUSORD(int type, const char* buf, int len, char* out)
{
    if ((type == TYPE_PLUS) && out) {
        int sz, sk;
        if ((sscanf(buf, "\r\n+USORD: %d,%d,", &sk, &sz) == 2) &&
            (buf[len-sz-2] == '\"') && (buf[len-1] == '\"')) {
            memcpy(out, &buf[len-1-sz], sz);
        }
    }
    return WAIT;
}

int MDMParser::socketRecv(int socket, char* buf, int len)
{
    int cnt = 0;
    TRACE("socketRecv(%d,,%d)\r\n", socket, len);
#ifdef MDM_DEBUG
    memset(buf, '\0', len);
#endif
    Timer timer;
    timer.start();
    while (len) {
        int blk = MAX_SIZE; // still need space for headers and unsolicited  commands
        if (len < blk) blk = len;
        bool ok = false;
        if (ISSOCKET(socket)) {
            if (_sockets[socket].connected) {
                if (_sockets[socket].pending < blk)
                    blk = _sockets[socket].pending;
                if (blk > 0) {
                    sendFormated("AT+USORD=%d,%d\r\n",_sockets[socket].handle, blk);
                    if (RESP_OK == waitFinalResp(_cbUSORD, buf)) {
                        _sockets[socket].pending -= blk;
                        len -= blk;
                        cnt += blk;
                        buf += blk;
                        ok = true;
                    }
                } else if (!TIMEOUT(timer, _sockets[socket].timeout_ms)) {
                    ok = (WAIT == waitFinalResp(NULL,NULL,0)); // wait for URCs
                } else {
                    len = 0;
                    ok = true;
                }
            } else {
                len = 0;
                ok = true;
            }
        }
        if (!ok) {
            TRACE("socketRecv: ERROR\r\n");
            return SOCKET_ERROR;
        }
    }
    TRACE("socketRecv: %d \"%*s\"\r\n", cnt, cnt, buf-cnt);
    return cnt;
}

int MDMParser::_cbUSORF(int type, const char* buf, int len, USORFparam* param)
{
    if ((type == TYPE_PLUS) && param) {
        int sz, sk, p, a,b,c,d;
        int r = sscanf(buf, "\r\n+USORF: %d,\"" IPSTR "\",%d,%d,",
            &sk,&a,&b,&c,&d,&p,&sz);
        if ((r == 7) && (buf[len-sz-2] == '\"') && (buf[len-1] == '\"')) {
            memcpy(param->buf, &buf[len-1-sz], sz);
            param->ip = IPADR(a,b,c,d);
            param->port = p;
        }
    }
    return WAIT;
}

int MDMParser::socketRecvFrom(int socket, IP* ip, int* port, char* buf, int len)
{
    int cnt = 0;
    TRACE("socketRecvFrom(%d,,%d)\r\n", socket, len);
#ifdef MDM_DEBUG
    memset(buf, '\0', len);
#endif
    Timer timer;
    timer.start();
    while (len) {
        int blk = MAX_SIZE; // still need space for headers and unsolicited commands
        if (len < blk) blk = len;
        bool ok = false;
        if (ISSOCKET(socket)) {
            if (_sockets[socket].pending < blk)
                blk = _sockets[socket].pending;
            if (blk > 0) {
                sendFormated("AT+USORF=%d,%d\r\n",_sockets[socket].handle, blk);
                USORFparam param;
                param.buf = buf;
                if (RESP_OK == waitFinalResp(_cbUSORF, &param)) {
                    _sockets[socket].pending -= blk;
                    *ip = param.ip;
                    *port = param.port;
                    len -= blk;
                    cnt += blk;
                    buf += blk;
                    len = 0; // done
                    ok = true;
                }
            } else if (!TIMEOUT(timer, _sockets[socket].timeout_ms)) {
                ok = (WAIT == waitFinalResp(NULL,NULL,0)); // wait for URCs
            } else {
                len = 0; // no more data and socket closed or timed-out
                ok = true;
            }
        }
        if (!ok) {
            TRACE("socketRecv: ERROR\r\n");
            return SOCKET_ERROR;
        }
    }
    timer.stop();
    timer.reset();
    TRACE("socketRecv: %d \"%*s\"\r\n", cnt, cnt, buf-cnt);
    return cnt;
}

int MDMParser::_findSocket(int handle) {
    for (int socket = 0; socket < NUMSOCKETS; socket++) {
        if (_sockets[socket].handle == handle)
            return socket;
    }
    return SOCKET_ERROR;
}

// ----------------------------------------------------------------

int MDMParser::_cbCMGL(int type, const char* buf, int len, CMGLparam* param)
{
    if ((type == TYPE_PLUS) && param && param->num) {
        // +CMGL: <ix>,...
        int ix;
        if (sscanf(buf, "\r\n+CMGL: %d,", &ix) == 1)
        {
            *param->ix++ = ix;
            param->num--;
        }
    }
    return WAIT;
}

int MDMParser::smsList(const char* stat /*= "ALL"*/, int* ix /*=NULL*/, int num /*= 0*/) {
    int ret = -1;
    sendFormated("AT+CMGL=\"%s\"\r\n", stat);
    CMGLparam param;
    param.ix = ix;
    param.num = num;
    if (RESP_OK == waitFinalResp(_cbCMGL, &param))
        ret = num - param.num;
    return ret;
}

bool MDMParser::smsSend(const char* num, const char* buf)
{
    bool ok = false;
    sendFormated("AT+CMGS=\"%s\"\r\n",num);
    if (RESP_PROMPT == waitFinalResp(NULL,NULL,150*1000)) {
        send(buf, strlen(buf));
        const char ctrlZ = 0x1A;
        send(&ctrlZ, sizeof(ctrlZ));
        ok = (RESP_OK == waitFinalResp());
    }
    return ok;
}

bool MDMParser::smsDelete(int ix)
{
    bool ok = false;
    sendFormated("AT+CMGD=%d\r\n",ix);
    ok = (RESP_OK == waitFinalResp());
    return ok;
}

int MDMParser::_cbCMGR(int type, const char* buf, int len, CMGRparam* param)
{
    if (param) {
        if (type == TYPE_PLUS) {
            if (sscanf(buf, "\r\n+CMGR: \"%*[^\"]\",\"%[^\"]", param->num) == 1) {
            }
        } else if ((type == TYPE_UNKNOWN) && (buf[len-2] == '\r') && (buf[len-1] == '\n')) {
            memcpy(param->buf, buf, len-2);
            param->buf[len-2] = '\0';
        }
    }
    return WAIT;
}

bool MDMParser::smsRead(int ix, char* num, char* buf, int len)
{
    bool ok = false;
    CMGRparam param;
    param.num = num;
    param.buf = buf;
    sendFormated("AT+CMGR=%d\r\n",ix);
    ok = (RESP_OK == waitFinalResp(_cbCMGR, &param));
    return ok;
}

// ----------------------------------------------------------------

int MDMParser::_cbCUSD(int type, const char* buf, int len, char* resp)
{
    if ((type == TYPE_PLUS) && resp) {
        // +USD: \"%*[^\"]\",\"%[^\"]\",,\"%*[^\"]\",%d,%d,%d,%d,\"*[^\"]\",%d,%d"..);
        if (sscanf(buf, "\r\n+CUSD: %*d,\"%[^\"]\",%*d", resp) == 1) {
            /*nothing*/
        }
    }
    return WAIT;
}

bool MDMParser::ussdCommand(const char* cmd, char* buf)
{
    bool ok = false;
    *buf = '\0';
    if (_dev.dev != DEV_LISA_C200) {
        sendFormated("AT+CUSD=1,\"%s\"\r\n",cmd);
        ok = (RESP_OK == waitFinalResp(_cbCUSD, buf));
    }
    return ok;
}

// ----------------------------------------------------------------

int MDMParser::_cbUDELFILE(int type, const char* buf, int len, void*)
{
    if ((type == TYPE_ERROR) && strstr(buf, "+CME ERROR: FILE NOT FOUND"))
        return RESP_OK; // file does not exist, so all ok...
    return WAIT;
}

bool MDMParser::delFile(const char* filename)
{
    bool ok = false;
    sendFormated("AT+UDELFILE=\"%s\"\r\n", filename);
    ok = (RESP_OK == waitFinalResp(_cbUDELFILE));
    return ok;
}

int MDMParser::writeFile(const char* filename, const char* buf, int len)
{
    bool ok = false;
    sendFormated("AT+UDWNFILE=\"%s\",%d\r\n", filename, len);
    if (RESP_PROMPT == waitFinalResp()) {
        send(buf, len);
        ok = (RESP_OK == waitFinalResp());
    }
    return ok ? len : -1;
}

int MDMParser::readFile(const char* filename, char* buf, int len)
{
    URDFILEparam param;
    param.filename = filename;
    param.buf = buf;
    param.sz = len;
    param.len = 0;
    sendFormated("AT+URDFILE=\"%s\"\r\n", filename, len);
    if (RESP_OK != waitFinalResp(_cbURDFILE, &param))
        param.len = -1;
    return param.len;
}

int MDMParser::_cbURDFILE(int type, const char* buf, int len, URDFILEparam* param)
{
    if ((type == TYPE_PLUS) && param && param->filename && param->buf) {
        char filename[48];
        int sz;
        if ((sscanf(buf, "\r\n+URDFILE: \"%[^\"]\",%d,", filename, &sz) == 2) &&
            (0 == strcmp(param->filename, filename)) &&
            (buf[len-sz-2] == '\"') && (buf[len-1] == '\"')) {
            param->len = (sz < param->sz) ? sz : param->sz;
            memcpy(param->buf, &buf[len-1-sz], param->len);
        }
    }
    return WAIT;
}

// ----------------------------------------------------------------
int MDMParser::_parseMatch(Pipe<char>* pipe, int len, const char* sta, const char* end)
{
    int o = 0;
    if (sta) {
        while (*sta) {
            if (++o > len)              return WAIT;
            char ch = pipe->next();
            if (*sta++ != ch)           return NOT_FOUND;
        }
    }
    if (!end)                           return o; // no termination
    // at least any char
    if (++o > len)                      return WAIT;
    pipe->next();
    // check the end
    int x = 0;
    while (end[x]) {
        if (++o > len)                  return WAIT;
        char ch = pipe->next();
        x = (end[x] == ch) ? x + 1 :
            (end[0] == ch) ? 1 : 0;
    }
    return o;
}

int MDMParser::_parseFormated(Pipe<char>* pipe, int len, const char* fmt)
{
    int o = 0;
    int num = 0;
    if (fmt) {
        while (*fmt) {
            if (++o > len)                  return WAIT;
            char ch = pipe->next();
            if (*fmt == '%') {
                fmt++;
                if (*fmt == 'd') { // numeric
                    fmt ++;
                    num = 0;
                    while (ch >= '0' && ch <= '9') {
                        num = num * 10 + (ch - '0');
                        if (++o > len)      return WAIT;
                        ch = pipe->next();
                    }
                }
                else if (*fmt == 'c') { // char buffer (takes last numeric as length)
                    fmt ++;
                    while (num --) {
                        if (++o > len)      return WAIT;
                        ch = pipe->next();
                    }
                }
                else if (*fmt == 's') {
                    fmt ++;
                    if (ch != '\"')         return NOT_FOUND;
                    do {
                        if (++o > len)      return WAIT;
                        ch = pipe->next();
                    } while (ch != '\"');
                    if (++o > len)          return WAIT;
                    ch = pipe->next();
                }
            }
            if (*fmt++ != ch)               return NOT_FOUND;
        }
    }
    return o;
}

int MDMParser::_getLine(Pipe<char>* pipe, char* buf, int len)
{
    int unkn = 0;
    int sz = pipe->size();
    int fr = pipe->free();
    if (len > sz)
        len = sz;
    while (len > 0)
    {
        static struct {
              const char* fmt;                              int type;
        } lutF[] = {
            { "\r\n+USORD: %d,%d,\"%c\"",                   TYPE_PLUS       },
            { "\r\n+USORF: %d,\"" IPSTR "\",%d,%d,\"%c\"",  TYPE_PLUS       },
            { "\r\n+URDFILE: %s,%d,\"%c\"",                 TYPE_PLUS       },
        };
        static struct {
              const char* sta;          const char* end;    int type;
        } lut[] = {
            { "\r\nOK\r\n",             NULL,               TYPE_OK         },
            { "\r\nERROR\r\n",          NULL,               TYPE_ERROR      },
            { "\r\n+CME ERROR:",        "\r\n",             TYPE_ERROR      },
            { "\r\n+CMS ERROR:",        "\r\n",             TYPE_ERROR      },
            { "\r\nRING\r\n",           NULL,               TYPE_RING       },
            { "\r\nCONNECT\r\n",        NULL,               TYPE_CONNECT    },
            { "\r\nNO CARRIER\r\n",     NULL,               TYPE_NOCARRIER  },
            { "\r\nNO DIALTONE\r\n",    NULL,               TYPE_NODIALTONE },
            { "\r\nBUSY\r\n",           NULL,               TYPE_BUSY       },
            { "\r\nNO ANSWER\r\n",      NULL,               TYPE_NOANSWER   },
            { "\r\n+",                  "\r\n",             TYPE_PLUS       },
            { "\r\n@",                  NULL,               TYPE_PROMPT     }, // Sockets
            { "\r\n>",                  NULL,               TYPE_PROMPT     }, // SMS
            { "\n>",                    NULL,               TYPE_PROMPT     }, // File
        };
        for (int i = 0; i < sizeof(lutF)/sizeof(*lutF); i ++) {
            pipe->set(unkn);
            int ln = _parseFormated(pipe, len, lutF[i].fmt);
            if (ln == WAIT && fr)
                return WAIT;
            if ((ln != NOT_FOUND) && (unkn > 0))
                return TYPE_UNKNOWN | pipe->get(buf, unkn);
            if (ln > 0)
                return lutF[i].type  | pipe->get(buf, ln);
        }
        for (int i = 0; i < sizeof(lut)/sizeof(*lut); i ++) {
            pipe->set(unkn);
            int ln = _parseMatch(pipe, len, lut[i].sta, lut[i].end);
            if (ln == WAIT && fr)
                return WAIT;
            if ((ln != NOT_FOUND) && (unkn > 0))
                return TYPE_UNKNOWN | pipe->get(buf, unkn);
            if (ln > 0)
                return lut[i].type | pipe->get(buf, ln);
        }
        // UNKNOWN
        unkn ++;
        len--;
    }
    return WAIT;
}

// ----------------------------------------------------------------
// Serial Implementation
// ----------------------------------------------------------------

/*! Helper Dev Null Device
    Small helper class used to shut off stderr/stdout. Sometimes stdin/stdout
    is shared with the serial port of the modem. Having printfs inbetween the
    AT commands you cause a failure of the modem.
*/
class DevNull : public Stream {
public:
    DevNull() : Stream(_name+1) { }             //!< Constructor
    void claim(const char* mode, FILE* file)
        { freopen(_name, mode, file); }         //!< claim a stream
protected:
    virtual int _getc()         { return EOF; } //!< Nothing
    virtual int _putc(int c)    { return c; }   //!< Discard
    static const char* _name;                   //!< File name
};
const char* DevNull::_name = "/null";  //!< the null device name
static      DevNull null;              //!< the null device

MDMSerial::MDMSerial(PinName tx /*= MDMTXD*/, PinName rx /*= MDMRXD*/,
            int baudrate /*= MDMBAUD*/,
#if DEVICE_SERIAL_FC
            PinName rts /*= MDMRTS*/, PinName cts /*= MDMCTS*/,
#endif
            int rxSize /*= 256*/, int txSize /*= 128*/) :
            SerialPipe(tx, rx, rxSize, txSize)
{
    if (rx == USBRX)
        null.claim("r", stdin);
    if (tx == USBTX) {
        null.claim("w", stdout);
        null.claim("w", stderr);
    }
    //mdm_powerOn(false);
    baud(baudrate);
#if DEVICE_SERIAL_FC
    if ((rts != NC) || (cts != NC))
    {
        Flow flow = (cts == NC) ? RTS :
                    (rts == NC) ? CTS : RTSCTS ;
        set_flow_control(flow, rts, cts);
        if (cts != NC) _dev.lpm = LPM_ENABLED;
    }
#endif
}

MDMSerial::~MDMSerial(void)
{
    powerOff();
    //mdm_powerOff();
}

int MDMSerial::_send(const void* buf, int len)
{
    return put((const char*)buf, len, true/*=blocking*/);
}

int MDMSerial::getLine(char* buffer, int length)
{
    return _getLine(&_pipeRx, buffer, length);
}
