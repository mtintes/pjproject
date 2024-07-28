/*
 * Copyright (C) 2008-2013 Teluu Inc. (http://www.teluu.com)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

Changes:
This is a copy of pjsua2_demo.cpp with some minor tweaks to make make a simple virtual phone.
You may have to change audio ports # for your system.
You can use ./pjsip-apps/bin/samples/.../auddemo to find your device numbers

 */
#include <pjsua2.hpp>
#include <iostream>
#include <pj/file_access.h>

#define THIS_FILE "shatterphone.cpp"

using namespace pj;

class MyEndpoint : public Endpoint
{
public:
    MyEndpoint() : Endpoint() {};
    virtual pj_status_t onCredAuth(OnCredAuthParam &prm)
    {
        PJ_UNUSED_ARG(prm);
        std::cout << "*** Callback onCredAuth called ***" << std::endl;
        /* Return PJ_ENOTSUP to use
         * pjsip_auth_create_aka_response()/<b>libmilenage</b> (default),
         * if PJSIP_HAS_DIGEST_AKA_AUTH is defined.
         */
        return PJ_ENOTSUP;
    }
};

class MyAccount;

class MyAudioMediaPort : public AudioMediaPort
{
    virtual void onFrameRequested(MediaFrame &frame)
    {
        // Give the input frame here
        frame.type = PJMEDIA_FRAME_TYPE_AUDIO;
        // frame.buf.assign(frame.size, 'c');
    }

    virtual void onFrameReceived(MediaFrame &frame)
    {
        PJ_UNUSED_ARG(frame);
        // Process the incoming frame here
    }
};

class MyCall : public Call
{
private:
    MyAccount *myAcc;
    AudioMediaPlayer *wav_player;
    AudioMediaPort *med_port;

public:
    MyCall(Account &acc, int call_id = PJSUA_INVALID_ID)
        : Call(acc, call_id)
    {
        wav_player = NULL;
        med_port = NULL;
        myAcc = (MyAccount *)&acc;
    }

    ~MyCall()
    {
        if (wav_player)
            delete wav_player;
        if (med_port)
            delete med_port;
    }

    virtual void onCallState(OnCallStateParam &prm);
    virtual void onCallTransferRequest(OnCallTransferRequestParam &prm);
    virtual void onCallReplaceRequest(OnCallReplaceRequestParam &prm);
    virtual void onCallMediaState(OnCallMediaStateParam &prm);
};

class MyAccount : public Account
{
public:
    std::vector<Call *> calls;

public:
    MyAccount()
    {
    }

    ~MyAccount()
    {
        std::cout << "*** Account is being deleted: No of calls="
                  << calls.size() << std::endl;

        for (std::vector<Call *>::iterator it = calls.begin();
             it != calls.end();)
        {
            delete (*it);
            it = calls.erase(it);
        }
    }

    void removeCall(Call *call)
    {
        for (std::vector<Call *>::iterator it = calls.begin();
             it != calls.end(); ++it)
        {
            if (*it == call)
            {
                calls.erase(it);
                break;
            }
        }
    }

    virtual void onRegState(OnRegStateParam &prm)
    {
        AccountInfo ai = getInfo();
        std::cout << (ai.regIsActive ? "*** Register: code=" : "*** Unregister: code=")
                  << prm.code << std::endl;
    }

    virtual void onIncomingCall(OnIncomingCallParam &iprm)
    {
        Call *call = new MyCall(*this, iprm.callId);
        CallInfo ci = call->getInfo();
        CallOpParam prm;

        std::cout << "*** Incoming Call: " << ci.remoteUri << " ["
                  << ci.stateText << "]" << std::endl;

        calls.push_back(call);
        char line[10], *dummy;
        dummy = fgets(line, sizeof(line), stdin);
        PJ_UNUSED_ARG(dummy);
        prm.statusCode = (pjsip_status_code)200;
        call->answer(prm);
    }
};

void MyCall::onCallState(OnCallStateParam &prm)
{
    PJ_UNUSED_ARG(prm);

    CallInfo ci = getInfo();
    std::cout << "*** Call: " << ci.remoteUri << " [" << ci.stateText
              << "]" << std::endl;

    if (ci.state == PJSIP_INV_STATE_DISCONNECTED)
    {
        // myAcc->removeCall(this);
        /* Delete the call */
        // delete this;
    }
}

void MyCall::onCallMediaState(OnCallMediaStateParam &prm)
{
    PJ_UNUSED_ARG(prm);
    CallInfo ci = getInfo();
    for (unsigned i = 0; i < ci.media.size(); i++)
    {
        if (ci.media[i].type == PJMEDIA_TYPE_AUDIO && getMedia(i))
        {
            AudioMedia *aud_med = (AudioMedia *)getMedia(i);

            // Connect the call audio media to sound device
            AudDevManager &mgr = Endpoint::instance().audDevManager();
            aud_med->startTransmit(mgr.getPlaybackDevMedia());
            mgr.getCaptureDevMedia().startTransmit(*aud_med);
        }
    }

    // This works for listing to the call and playing a wav file
    // but not microphone
    // PJ_UNUSED_ARG(prm);

    // CallInfo ci = getInfo();
    // AudioMedia aud_med;
    // AudioMedia& play_dev_med =
    //     MyEndpoint::instance().audDevManager().getPlaybackDevMedia();
    // AudioMedia& cap_dev_med =
    //     MyEndpoint::instance().audDevManager().getCaptureDevMedia();

    // try {
    //     // Get the first audio media
    //     aud_med = getAudioMedia(-1);
    // } catch(...) {
    //     std::cout << "Failed to get audio media" << std::endl;
    //     return;
    // }

    // // if (!wav_player) {
    // //     wav_player = new AudioMediaPlayer();
    // //     try {
    // //         wav_player->createPlayer(
    // //             "input.16.wav", 0);
    // //     } catch (...) {
    // //         std::cout << "Failed opening wav file" << std::endl;
    // //         delete wav_player;
    // //         wav_player = NULL;
    // //     }
    // // }

    // // This will connect the wav file to the call audio media
    // // if (wav_player)
    // //     wav_player->startTransmit(aud_med);

    // if (!med_port) {
    //     med_port = new MyAudioMediaPort();

    //     MediaFormatAudio fmt;
    //     fmt.init(PJMEDIA_FORMAT_PCM, 16000, 1, 20000, 16);

    //     med_port->createPort("med_port", fmt);

    //     // Connect the media port to the call audio media in both directions
    //     med_port->startTransmit(aud_med);
    //     aud_med.startTransmit(*med_port);
    // }

    // // And this will connect the call audio media to the sound device/speaker
    // aud_med.startTransmit(play_dev_med);
    // aud_med.startTransmit(cap_dev_med);
}

void MyCall::onCallTransferRequest(OnCallTransferRequestParam &prm)
{
    /* Create new Call for call transfer */
    prm.newCall = new MyCall(*myAcc);
}

void MyCall::onCallReplaceRequest(OnCallReplaceRequestParam &prm)
{
    /* Create new Call for call replace */
    prm.newCall = new MyCall(*myAcc);
}

string getPhoneInput()
{
    string phonenumber;
    char line[10], *dummy;
    std::cout << "Enter a phone number:" << std::endl;
    bool noMoreCharacters = true;
    while(noMoreCharacters){
        char c = getchar();
        if(c == '\n'){
            noMoreCharacters = false;
        } else {
            phonenumber += c;
        }
    }
    std::cout << "phone number:" << phonenumber << std::endl;
    return phonenumber;
}

static void mainProg1(MyEndpoint &ep)
{
    char * asteriskIp = getenv("ASTERISK_IP");
    if(asteriskIp == NULL){
        std::cout << "Please set the ASTERISK_IP environment variable" << std::endl;
        return;
    }
    char * sipId = getenv("SIP_ID");
    if(sipId == NULL){
        std::cout << "Please set the SIP_ID environment variable" << std::endl;
        return;
    }
    char * sipPassword = getenv("SIP_PASSWORD");
    if(sipPassword == NULL){
        std::cout << "Please set the SIP_PASSWORD environment variable" << std::endl;
        return;
    }

    // Init library
    EpConfig ep_cfg;
    ep_cfg.logConfig.level = 5;
    ep.libInit(ep_cfg);

    // Transport
    TransportConfig tcfg;
    tcfg.port = 5060;
    ep.transportCreate(PJSIP_TRANSPORT_UDP, tcfg);

    // Start library
    ep.libStart();
    std::cout << "*** PJSUA2 STARTED ***" << std::endl;

    // Add account
    AccountConfig acc_cfg;
    acc_cfg.idUri = "sip: "+ string(sipId) +" @" + string(asteriskIp);
    acc_cfg.regConfig.registrarUri = "sip:" + string(asteriskIp);

#if PJSIP_HAS_DIGEST_AKA_AUTH
    AuthCredInfo aci("Digest", "*", "test", PJSIP_CRED_DATA_EXT_AKA | PJSIP_CRED_DATA_PLAIN_PASSWD, "passwd");
    aci.akaK = "passwd";
#else
    AuthCredInfo aci("digest", "*", string(sipId), 0, string(sipPassword));
#endif

    acc_cfg.sipConfig.authCreds.push_back(aci);
    MyAccount *acc(new MyAccount);
    try
    {
        acc->create(acc_cfg);
    }
    catch (...)
    {
        std::cout << "Adding account failed" << std::endl;
    }

    pj_thread_sleep(2000);

    // probably need to make this configurable
    MyEndpoint::instance().audDevManager().setCaptureDev(8);
    MyEndpoint::instance().audDevManager().setPlaybackDev(8);
    int counter = MyEndpoint::instance().audDevManager().getDevCount();

    std::cout << "Device count: " << counter << std::endl;
    pj_status_t status;
    for (int i = 0; i < counter; ++i)
    {
        pjmedia_aud_dev_info info;

        status = pjmedia_aud_dev_get_info(i, &info);
        if (status != PJ_SUCCESS)
            continue;

        PJ_LOG(3, (THIS_FILE, " %2d: %s [%s] (%d/%d)",
                   i, info.driver, info.name, info.input_count, info.output_count));
    }

    CallOpParam prm(true);
    prm.opt.audioCount = 1;
    prm.opt.videoCount = 0;

    bool onHook = true;
    // Hangup all calls
    while (true)
    {
        char * val = getenv("ONHOOK");
        if(val != NULL){
            onHook = true;
        } else {
            onHook = false;
        }

        if (onHook) 
        {
            pj_thread_sleep(10);
        }
        else
        {
            string phonenumber = getPhoneInput();
            Call *call = new MyCall(*acc);
            acc->calls.push_back(call);
            call->makeCall("sip:"+phonenumber+"@" + string(asteriskIp), prm);
        }
    }
    ep.hangupAllCalls();

    // Destroy library
    std::cout << "*** PJSUA2 SHUTTING DOWN ***" << std::endl;
    delete acc; /* Will delete all calls too */
}

extern "C" int main()
{
    int ret = 0;
    MyEndpoint ep;

    try
    {
        ep.libCreate();

        mainProg1(ep);

        ret = PJ_SUCCESS;
    }
    catch (Error &err)
    {
        std::cout << "Exception: " << err.info() << std::endl;
        ret = 1;
    }

    try
    {
        ep.libDestroy();
    }
    catch (Error &err)
    {
        std::cout << "Exception: " << err.info() << std::endl;
        ret = 1;
    }

    if (ret == PJ_SUCCESS)
    {
        std::cout << "Success" << std::endl;
    }
    else
    {
        std::cout << "Error Found" << std::endl;
    }

    return ret;
}
