#ifndef __webserver_h__
#define __webserver_h__

#include "peer.h"
#include "thread.h"

/* include websockets */
#define sha1 sha1_
#define assert assert_
#include "websocket.h"
#define sha1_ sha1
#define assert_ assert

extern int listen_port;
extern peer_session_t peers[];
extern int stun_binding_response_count;

static char g_chatlog[2048016];

extern int sdp_prefix_set(const char*);

struct webserver_state {
    char inip[64];
    int running;
    int peer_index_sdp_last;
};
extern struct webserver_state webserver;

enum websocket_state {
    WS_STATE_INITIAL = 1,
    WS_STATE_INITPEER = 2,
    WS_STATE_WRITESDP = 3,
    WS_STATE_JOINROOM = 4,
    WS_STATE_ROOMPOST = 5,
    WS_STATE_READING = 6,
    WS_STATE_EXIT = 7
};

typedef struct {
    int sock;
    pthread_t ws_thread;
    int ws_peeridx;
    int state;
    char websocket_accept_response[512];
    char* pbody;
    char roomname[256];
} webserver_worker_args;

static char *tag_icecandidate = "%$RTCICECANDIDATE$%";
static char *tag_room_ws = "%$ROOMNAME$%";
static char* tag_joinroom_ws = "POST /join/";
static char* tag_msgroom_ws = "POST /message/domain17/";


char*
macro_str_expand(char* buf, char* tag, char* replace)
{
    char* ret = buf;
    while(1)
    {
        ret = buf;

        /* macros */
        char *pmac = strstr(buf, tag);
        int expandlen = strlen(replace);
        if(pmac)
        {
            size_t realloc_len = strlen(buf) + expandlen + 1;
            char* tmp = malloc(realloc_len);
            if(tmp)
            {
                ret = tmp;
                memset(ret, 0, realloc_len);

                strncpy(ret, buf, pmac - buf);
                pmac += strlen(tag);
                strcat(ret, replace);
                strcat(ret, pmac);
                free(buf);
                buf = ret;
                continue;
            }
        }
        break;
    }
    return ret;
}

void
chatlog_append(const char* pchatmsg)
{ 
    char *pto = g_chatlog, *pfrom = g_chatlog;
    while(*pfrom && strlen(pfrom) + strlen(pchatmsg) >= sizeof(g_chatlog)-1) pfrom++;
    memmove(pto, pfrom, strlen(pfrom));
    strcat(pto, pchatmsg);

    file_write2(g_chatlog, strlen(g_chatlog), "chatlog.txt");
}


void
chatlog_reload()
{
    int file_buf_len = 0;
    memset(g_chatlog, 0, sizeof(g_chatlog));
    char* file_buf = file_read("chatlog.txt", &file_buf_len);
    if(file_buf) strcpy(g_chatlog, file_buf);
}

static void*
websocket_worker(void* p)
{
    webserver_worker_args* args = (webserver_worker_args*) p;
    struct {
        char* out;
        char* in;
        size_t out_len;
        size_t in_len;
    } ws_buffer;
    const size_t buf_size = 4096;
    int state;
    char* sdp = NULL;
    unsigned int icecandidates[16];
    char room_id_msg[256] = {0};
    int icecandidates_n = 0;

    peer_session_t* peer = NULL;

    thread_init();

    memset(&ws_buffer, 0, sizeof(ws_buffer));

    do
    {
        int sock;
        struct sockaddr_in sa;
        socklen_t sa_len;
        int flags = 0;

        memset(&sa, 0, sizeof(sa));
        sa_len = sizeof(sa);

        sock = args->sock;
        state = args->state;

        if(args->pbody)
        {
            printf("WS POST with body:\n%s\n", args->pbody);
        }

        if(sock >= 0)
        {
            char recvbuf[buf_size];
            char wsdecoded[buf_size];
            char sendbuf[buf_size];
            size_t send_len = 0;

            char value[buf_size];
            int value_len;
            int timeout_ms = 1000;
            int timeout_counter = 10;

            printf("%s:%d websocket connection (%s:%d)\n", __func__, __LINE__, inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));

            memset(recvbuf, 0, sizeof(recvbuf));
            memset(wsdecoded, 0, sizeof(wsdecoded));

            char *roff = recvbuf;
            unsigned int recv_left = sizeof(recvbuf)-1;

            while(state != WS_STATE_EXIT && (!peer || (time(NULL) - peer->time_pkt_last < 10)))
            {
                int r = 0;

                if(state == WS_STATE_READING) {

                    if(waitsocket(sock, timeout_ms / 1000, 0) == 0)
                    {
                        timeout_counter--;
                    }
                    else
                    {
                        timeout_counter = 30;
                        r = recv(sock, roff, recv_left > 0? 1: 0, flags);
                        if(r <= 0 || r > recv_left)
                        {
                            state = WS_STATE_EXIT; 
                            break;
                        }

                        roff += r;
                        recv_left -= r;
                    }
                }

                if(state == WS_STATE_READING && r > 0)
                {
                    uint8_t* decodedbuf = wsdecoded;
                    size_t decodedbuf_len = sizeof(wsdecoded)-1;

                    int frame_type =
                        wsParseInputFrame(recvbuf, roff-recvbuf,
                                          &decodedbuf, &decodedbuf_len);
                    switch(frame_type)
                    {
                    case WS_INCOMPLETE_FRAME:
                        // keep reading
                        continue;

                    case WS_TEXT_FRAME:
                        printf("websocket msg<-:\n%s\n", decodedbuf);
                        {
                             value_len = str_read_from_key("\\\"candidate\\\" : \\\"", decodedbuf, value, "\\", buf_size-1, 0);
                             while(value_len > 0 && icecandidates_n < 16) {
                                 char raddr_buf[64], rport_buf[64], cand_buf[64];
                                 if(str_read_from_key("raddr ", value, raddr_buf, " ", sizeof(raddr_buf)-1, 0) <= 0) break;
                                 if(str_read_from_key("rport ", value, rport_buf, " ", sizeof(rport_buf)-1, 0) <= 0) break;
                                 if(str_read_from_key("candidate:", value, cand_buf, " ", sizeof(cand_buf)-1, 0) <= 0) break;

                                 icecandidates[icecandidates_n++] = atoi(cand_buf);
                                 break;
                            }

                            value_len = str_read_from_key("\\\"sdp\\\":\\\"", decodedbuf, value, "\\\"", buf_size-1, 0);
                            if(value_len > 0)
                            {
                                sdp = strdup(decodedbuf);
                                if(sdp) sdp = str_replace_nested_escapes(sdp);
 
                                state = WS_STATE_INITPEER;
                            }

                            value_len = str_read_from_key("\"cmd\" : \"register\"", decodedbuf, value, "}", buf_size-1, 0);
                            if(value_len > 0)
                            {
                                value_len = str_read_from_key("\"roomid\" : \"", value, room_id_msg, "\"", sizeof(room_id_msg)-1, 0);
                            }
                        }

                    default:
                        roff = recvbuf;
                        recv_left = sizeof(recvbuf)-1;
                        memset(recvbuf, 0, sizeof(recvbuf));
                    }
                }

                if(state == WS_STATE_INITPEER)
                {
                    state = WS_STATE_READING;

                    int p;
                    for(p = 0; p < MAX_PEERS; p++)
                    {
                        if(!peers[p].alive)
                        {
                            peer = &peers[p];
                            break;
                        }
                    }
                 
                    if(peer)
                    {
                        peer_init(peer, PEER_INDEX(peer));

                        int file_buf_len = 0;
                        char* file_buf = file_read("content/apprtc/init.txt", &file_buf_len);

                        if(file_buf_len > 0)
                        {
                            char* offersdp = strdup(file_buf);
                            if(offersdp) offersdp = str_replace_nested_escapes(offersdp);
                            strncpy(peer->sdp.offer, offersdp, sizeof(peer->sdp.offer)-1);
 
                            strcpy(peer->stun_ice.ufrag_offer, sdp_read(offersdp, "a=ice-ufrag:"));

                            free(offersdp);
                        }

                        if(sdp)
                        {
                            char *psep = strchr(room_id_msg, '@');
                            *psep = '\0'; psep++;
                            sprintf(peer->sdp.answer, "a=myname=%s\r\na=watch=%s\r\n%s", room_id_msg, psep, sdp);
                            peer->stun_ice.controller = 0;
                            if(icecandidates_n > 0) peer->stun_ice.candidate_id = icecandidates[icecandidates_n-1];

                            free(sdp);
                            sdp = NULL;
                        }

                        // mark -- wait for peer to be initialized
                        peer->alive = 1;
                        peer->restart_needed = 1;

                        while(!peer->restart_done) usleep(1000);
                        peer->alive = 1;
                        peer->time_pkt_last = time(NULL);

                        peer->restart_needed = 0;
                    }
                }

                if(state == WS_STATE_INITIAL || state == WS_STATE_JOINROOM || state == WS_STATE_WRITESDP | state == WS_STATE_ROOMPOST)
                {
                    if(state == WS_STATE_INITIAL) {
                        
                        sprintf(sendbuf, "HTTP/1.1 101 Switching Protocols\r\nUpgrade: websocket\r\nConnection: Upgrade\r\nSec-WebSocket-Accept: %s\r\n\r\n", args->websocket_accept_response);
                        send_len = strlen(sendbuf);
                        state = WS_STATE_READING;
                    }
                    else if(state == WS_STATE_JOINROOM) {
                        chatlog_append("appRTC joining\n");
                       
                        int file_buf_len = 0;
                        char* file_buf = file_read("content/apprtc/init.txt", &file_buf_len);

                        if(file_buf_len > 0)
                        {
                            char stuncandidate_js[256];
                            char* response = strdup(file_buf);
                            sprintf(stuncandidate_js, "candidate:1 1 UDP 1234 %s "
                                "%d typ host",
                                get_config("udpserver_addr="), listen_port);
                            response = macro_str_expand(response,
                                    tag_icecandidate, stuncandidate_js);

                            response = macro_str_expand(response, tag_room_ws, args->roomname);

                            sprintf(sendbuf, "HTTP/1.0 200 OK\r\nContent-Length: %lu\r\nContent-Type: text/plain\r\n\r\n%s",
                                    strlen(response), response);
                            send_len = strlen(sendbuf);
                            free(response);
                            free(file_buf);
                        }
                        state = WS_STATE_EXIT;
                    }
                    else if(state == WS_STATE_ROOMPOST) {
                        sdp = strdup(args->pbody);
                        if(sdp) sdp = str_replace_nested_escapes(sdp);

                        state = WS_STATE_INITPEER;
                    }
                    else if(state == WS_STATE_WRITESDP) {
                        int file_buf_len = 0;
                        char* file_buf = file_read("content/apprtc/offer.txt", &file_buf_len);
                        if(file_buf_len > 0) {
                            strcpy(sendbuf, file_buf);
                            send_len = file_buf_len;
                            free(file_buf);
                        }

                        state = WS_STATE_READING;
                    }
                }

                if(send_len > 0) {
                    printf("websocket msg->:\n%s\n", sendbuf);
                    send(sock, sendbuf, send_len, 0);
                }
                send_len = 0;
            }

            printf("websocket_worker shutting down\n");
            shutdown(sock, SHUT_WR);
            close(sock);
            sock = -1;
        }
    }
    while(0);

    if(sdp) free(sdp);

    if(args) {
        if(args->pbody) free(args->pbody);
        free(args);
    }

    return NULL;
}


static void*
webserver_worker(void* p)
{
    int r;
    char *page_buf_welcome = "<html><p>Welcome</p></html>";
    char *page_buf_400 = "<html>Huh?<br><a href='/index.html'>index.html</a></html>";
    char *page_buf_sdp_uploaded = "<html><button><body onload='window.location=\"content/uploadDone.html\";'>redirecting...</body></html>";
    char *page_buf_redirect_chat = "<html><body onload='window.location=\"content/peersPopup.html\";'>redirecting...</body></html>";
    char *page_buf_redirect_back = "<html><body onload='location=\"/content/chat.html\";'>redirecting...</body></html>";
    char *page_buf_redirect_subscribe = "<html><body onload='location=\"/content/iframe_channel.html\";'>redirecting...</body></html>";
    char *ok_hdr = "HTTP/1.0 200 OK\r\n";
    char *content_type = "";
    char *fail_hdr = "HTTP/1.0 200 OK\r\nContent-Type: text/html\r\n\r\n";
    char *content_type_html = "Content-Type: text/html\r\n\r\n";
    char* content_length_hdr = "Content-Length: ";
    char *tag_hostname = "%$HOSTNAME$%";
    char *tag_peerdynamicjs = "%$PEERDYNAMICJS$%";
    char *tag_urlargs = "%$URLARGUMENTS$%";
    char *tag_webport = "%$WEBPORT$%";
    char *tag_rtpport = "%$RTPPORT$%";
    char *tag_peerlisthtml = "%$PEERLISTHTML$%";
    char *tag_peerlisthtml_options = "%$PEERLISTHTMLOPTIONS$%";
    char *tag_peerlist_jsarray = "%$PEERLISTJSARRAY$%";
    char *tag_stunconfig_js = "%$STUNCONFIGJS$%";
    char *tag_chatlogvalue = "%$CHATLOGTEXTAREAVALUE$%";
    char *tag_watchuser = "watch?user=";
    char *tag_login = "login.html";
    char *tag_login_apprtc = "/login/";
    char *tag_logout = "logout.html";
    char *tag_authcookie = "%$AUTHCOOKIE$%";
    const size_t buf_size = 4096;
    int use_user_fragment_prefix = 1;
    webserver_worker_args* args = (webserver_worker_args*) p;
    unsigned int content_len = 0;
    char listen_port_str[64];
    char cookie[256], cookieset[256];
    char ws_header_buf[256];
    peer_session_t* peer_found_via_cookie = NULL;

    memset(cookie, 0, sizeof(cookie));

    sprintf(cookieset, "%02x%02x%02x%02x", rand() % 0xff, rand() % 0xff, rand() % 0xff, rand() % 0xff);

    thread_init();

    sprintf(listen_port_str, "%d", listen_port);

    do
    {
        int sock;
        struct sockaddr_in sa;
        socklen_t sa_len;
        int flags = 0;

        memset(&sa, 0, sizeof(sa));
        sa_len = sizeof(sa);

        sock = args->sock;
        if(sock >= 0)
        {
            char recvbuf[buf_size];

            printf("%s:%d connection (%s:%d)\n", __func__, __LINE__, inet_ntoa(sa.sin_addr), ntohs(sa.sin_port));

            memset(recvbuf, 0, sizeof(recvbuf));

            char *roff = recvbuf;
            unsigned int recv_left = sizeof(recvbuf)-1;
            int timed_out = 0;
            int do_shutdown = 1;
            int timeout_ms = 10000;
            while(1)
            {
                /* new request */
                /* TODO: always indicate connection-close by shutting-down immediately */
                if(waitsocket(sock, timeout_ms / 1000, 0) == 0)
                {
                    printf("%s:%d timed out\n", __func__, __LINE__);

                    do_shutdown = 1;
                    timed_out = 1;
                }

                r = recv(sock, roff, recv_left, flags);
                if(r <= 0) break;

                const char *cookietoken;
                char* pcookie;

                cookietoken = "authCookieJS12242016=";
                pcookie = strstr(recvbuf, cookietoken);

                memset(cookie, 0, sizeof(cookie));
                if(pcookie)
                {
                    pcookie += strlen(cookietoken);
                    str_read(pcookie, cookie, ";\r\n\t", sizeof(cookie));
                    strcpy(cookieset, cookie);
                }

                char* pcontent_len = strstr(recvbuf, content_length_hdr);
                if(pcontent_len && content_len == 0)
                {
                    pcontent_len += strlen(content_length_hdr);
                    sscanf(pcontent_len, "%u", &content_len);
                }

                char* phdr_end = strstr(recvbuf, "\r\n\r\n");
                if(phdr_end && content_len == 0) break;

                roff += r; 
                recv_left -= r;

                /* if >= content-length received, break */
                unsigned int body_len = (roff-recvbuf) - (phdr_end-recvbuf) - 4;
                if(content_len > 0 && body_len >= content_len) break;
            }

            if(strlen(recvbuf) > 0)
            {
                char path[buf_size];
                char url_args[256];
                char *purl = NULL;
                char *pbody = NULL;
                char *phttpheaders = NULL;
                char *pend = NULL;
                char *response = NULL;
                int response_binary = 0;
                unsigned int file_buf_len = 0;
                char *file_buf = NULL;
                int cmd_post = 0;
                char cookie_hdr[256];
                int sidx;
                int become_ws = 0;

                memset(url_args, 0, sizeof(url_args));
                cookie_hdr[0] = '\0';

                purl = recvbuf;
                if(strncmp(recvbuf, "GET ", 4) == 0) {
                    purl = recvbuf+4;
                }
                else if(strncmp(recvbuf, tag_joinroom_ws, strlen(tag_joinroom_ws)) == 0) {
                    char *proomname = purl + strlen(tag_joinroom_ws);
                    str_read(proomname, args->roomname, "\r\n ", sizeof(args->roomname));
                    purl = recvbuf+5;
                    // parse room name
                    args->state = WS_STATE_JOINROOM;
                    become_ws = 1;
                }
                else if(strncmp(recvbuf, tag_msgroom_ws, strlen(tag_msgroom_ws)) == 0) {
                    purl = recvbuf+5;
                    args->state = WS_STATE_ROOMPOST;
                    become_ws = 1;
                }
                else if(strncmp(recvbuf, "POST ", 5) == 0) {
                    purl = recvbuf+5;
                    cmd_post = 1;
                }
                else {
                    continue;
                }

                response = strdup(page_buf_400);

                char *e = purl;
                char *pargs = NULL;
                while (e-purl < (sizeof(path)-1) && *e != '\0' && *e != '\r' && *e && *e != '\n' && *e != ' ' && *e != '?') e++;
                if(*e == '?') {
                    pargs = e+1;
                }
                *e = '\0';

                pbody = e+1;
                while(*pbody == '\r' || *pbody == '\n') pbody++;
                phttpheaders = pbody;
                if(*pbody != '\0')
                {
                    pbody = strstr(pbody, "\r\n\r\n");
                    if(pbody) pbody += 4;
                    else pbody = e+1;
                }

                pend = pbody;
                while(*pend) pend++;

                printf("%s:%d webserver received:\n----------------\n"
                       "%s\n---------------%s\n"
                       "---------------\n", __func__, __LINE__,
                       recvbuf, phttpheaders);

                if(pargs)
                {
                    if(strncmp(pargs, "args=", 5) == 0)
                    {
                        pargs += 5;

                        char* pargs_end = pargs;
                        while(*pargs_end && *pargs_end != ' ') pargs_end++;

                        if(pargs_end-pargs < sizeof(url_args)-1) strncpy(url_args, pargs, pargs_end-pargs);
                    }
                }

                peer_found_via_cookie = peer_find_by_cookie(cookie);
                printf("peer_found_via_cookie=%s\n", peer_found_via_cookie!=0? "yes" : "no");

                if(!cmd_post)
                {
                    sprintf(path, "./%s", purl);
                     
                    file_buf = file_read(path, &file_buf_len);

                    printf("%s:%d webserver GET for file (%s):\n\t%s\n", __func__, __LINE__, file_buf? "": "failed", path);

                    if(strncmp(purl, "/ws", 3) == 0) { become_ws = 1; args->state = WS_STATE_INITIAL; }

                    if(!file_buf && become_ws)
                    {
                        args->pbody = strdup(pbody);
                        strcpy(args->websocket_accept_response,
                               websocket_accept_header(phttpheaders, ws_header_buf));

                        if(args->state == WS_STATE_INITIAL)
                        {
                             printf("peer[%d] logging in via websocket\n", sidx);
                        }

                        if(!args->ws_thread) pthread_create(&args->ws_thread, NULL, websocket_worker, args);
                        free(response);
                        return NULL;
                    }
                    else if(!file_buf || strcmp(purl, "/") == 0)
                    {
                        send(sock, ok_hdr, strlen(ok_hdr), flags);
                        send(sock, content_type_html, strlen(content_type_html), flags);
                        send(sock, page_buf_400, strlen(page_buf_400), flags);
                        timed_out = 1;
                    }
                    else
                    {
                        free(response);
                        response = file_buf;

                        if(strstr(purl, ".js")) content_type = "Content-Type: text/javascript\r\n\r\n";
                        else if(strstr(purl, ".html")) content_type = content_type_html;
                        else if(strstr(purl, ".css")) content_type = "Content-Type: text/css\r\n\r\n";
                        else if(strstr(purl, ".jpg")) { response_binary = 1; content_type = "Content-Type: image/jpeg\r\n\r\n"; }
                        else if(strstr(purl, ".gif")) { response_binary = 1; content_type = "Content-Type: image/gif\r\n\r\n"; }
                        else if(strstr(purl, tag_watchuser)) content_type = content_type_html;
                        else content_type = content_type = "Content-Type: application/octet-stream\r\n\r\n";
                    }

                    if(strstr(purl, tag_login) && strlen(url_args) > 0)
                    {
                        printf("cookie=%s\n", cookie);
                        
                        for(sidx = 0; sidx < MAX_PEERS && !peer_found_via_cookie; sidx++) {
                            if(!peers[sidx].alive) {
                                printf("peer[%d] logging in\n", sidx);

                                peer_init(&peers[sidx], sidx);

                                peers[sidx].time_pkt_last = time(NULL);
                                peers[sidx].alive = 1;

                                peer_cookie_init(&peers[sidx], cookie);
                                strcpy((char*)&peers[sidx].name, url_args);
                                chatlog_append("login:"); chatlog_append(peers[sidx].name); chatlog_append("\n");
                                strcat(peers[sidx].http.dynamic_js, "myUsername = '");
                                strcat(peers[sidx].http.dynamic_js, peers[sidx].name);
                                strcat(peers[sidx].http.dynamic_js, "';\n");
                                break;
                            }
                        }
                    }

                    if(strstr(purl, tag_logout))
                    {
                        if(peer_found_via_cookie)
                        {
                            chatlog_append("logged out:"); chatlog_append(peer_found_via_cookie->name); chatlog_append("\n");
                            peer_init(peer_found_via_cookie, PEER_INDEX(peer_found_via_cookie));
                            peer_cookie_init(peer_found_via_cookie, "");
                            peer_found_via_cookie = NULL;
                        }
                    }

                    /* macros */
                    if(peer_found_via_cookie)
                    {
                        response = macro_str_expand(response, tag_peerdynamicjs, peer_found_via_cookie->http.dynamic_js);
                    }
                    else
                    {
                        response = macro_str_expand(response, tag_peerdynamicjs, "/* no cookie found */");
                    }
                    response = macro_str_expand(response, tag_hostname, get_config("udpserver_addr="));
                    response = macro_str_expand(response, tag_urlargs, url_args);
                    response = macro_str_expand(response, tag_webport, get_config("webserver_port="));
                    response = macro_str_expand(response, tag_rtpport, listen_port_str);
                    
                    if(1)
                    {
                        const char *onClickMethod = "onPeerClick";
                        char peer_list_html[buf_size];
                        memset(peer_list_html, 0, sizeof(peer_list_html));
                        char line[buf_size];
                        int i;
                        int num_peers = 0; 
                        for(i = 0; i < MAX_PEERS; i++)
                        {
                            if(peers[i].alive)
                            {
                                num_peers++;

                                char key_buf[1024];
                                hex_print(key_buf, peers[i].dtls.master_key_salt, 8);
                                sprintf(line, "<input type='checkbox' %s onClick='%s(\"%s\", this);'>[%d]:%s (address:%s:%d bytes:%u id:%d master_key:%s)</input><br>\n",
                                        peers[i].recv_only? "disabled": "",
                                        (char*)onClickMethod,
                                        peers[i].name, i, peers[i].name,
                                        inet_ntoa(peers[i].addr.sin_addr),
                                        ntohs(peers[i].addr.sin_port),
                                        peers[i].rtp_buffered_total,
                                        peers[i].id,
                                        key_buf);
                                strncat(peer_list_html, line, sizeof(peer_list_html)-strlen(peer_list_html)-1);
                            }
                        }                       
                        response = macro_str_expand(response, tag_peerlisthtml, peer_list_html);
                    }

                    if(1)
                    {
                        char peer_list_html[buf_size];
                        memset(peer_list_html, 0, sizeof(peer_list_html));
                        char line[buf_size];
                        int i;
                        int num_peers = 0; 
                        for(i = 0; i < MAX_PEERS; i++)
                        {
                            if(peers[i].alive)
                            {
                                num_peers++;

                                char key_buf[1024];
                                hex_print(key_buf, peers[i].dtls.master_key_salt, 8);
                                sprintf(line, "<option value=\"%s\">%s (%s:%d)</option>\n", peers[i].name, peers[i].name, inet_ntoa(peers[i].addr.sin_addr), ntohs(peers[i].addr.sin_port));
                                strncat(peer_list_html, line, sizeof(peer_list_html)-strlen(peer_list_html)-1);
                            }
                        }                       
                        response = macro_str_expand(response, tag_peerlisthtml_options, peer_list_html);
                    }

                    if(1)
                    {
                        char peer_list_html[buf_size];
                        memset(peer_list_html, 0, sizeof(peer_list_html));
                        char line[buf_size];
                        int i;
                        int num_peers = 0;
                        int first_entry = 1;
                        strcat(peer_list_html, "var peerList = [");
                        for(i = 0; i < MAX_PEERS; i++)
                        {
                            if(peers[i].alive)
                            {
                                num_peers++;

                                char key_buf[1024];
                                hex_print(key_buf, peers[i].dtls.master_key_salt, 8);
                                sprintf(line, "%s{'name': '%s', 'id': '%d', 'addr': '%s:%u', 'key': '%s', 'recvonly': %s }",
                                        (first_entry? "": ","), peers[i].name, peers[i].id, inet_ntoa(peers[i].addr.sin_addr), ntohs(peers[i].addr.sin_port),
                                        key_buf, (peers[i].recv_only? "true": "false"));
                                strncat(peer_list_html, line, sizeof(peer_list_html)-strlen(peer_list_html)-1);
                                first_entry = 0;
                            }
                        }
                        strcat(peer_list_html, "];\n");
                        response = macro_str_expand(response, tag_peerlist_jsarray, peer_list_html);
                    }

                    if(1)
                    {
                        char stun_config_js[256];
                        sprintf(stun_config_js, "{ \"iceServers\": [{ \"url\": \"stun:%s:%s\" }] }", get_config("udpserver_addr="), listen_port_str);
                        response = macro_str_expand(response, tag_stunconfig_js, /*stun_config_js*/ "null");
                    }

                    if(1)
                    {
                        char stuncandidate_js[256];
                        sprintf(stuncandidate_js, "candidate:1 1 UDP 1234 %s %s typ host", get_config("udpserver_addr="), listen_port_str);
                        response = macro_str_expand(response, tag_icecandidate, stuncandidate_js);
                    }

                    if(1)
                    {
                        response = macro_str_expand(response, tag_chatlogvalue, g_chatlog);
                    }

                    if(1)
                    {
                        response = macro_str_expand(response, tag_authcookie, cookieset);
                    }
                }
                else
                {
                    sprintf(path, "./%s", purl);

                    printf("%s:%d webserver POST for file (content_len:%d):\n\t%s\n", __func__, __LINE__, content_len, purl);

                    if(strcmp(purl, "/"FILENAME_SDP_ANSWER) == 0)
                    {
                        /* create temp file, decode, rename for worker thread to pick up/read and remove */
                        char user_fragment[64];
                        char tmp_filename[128];
                        char tmp[256];
                        char* sdp = strdup(pbody);
                        sdp = sdp_decode(sdp);

                        // anonymous peers don't reserve a slot
                        if(strstr(sdp, "a=recvonly") != NULL) peer_found_via_cookie = NULL;

                        int sidx_offset = 0;
                        if(str_read_unsafe(sdp, "a=channeloffset=", 0)) {
                            sidx_offset = atoi(str_read_unsafe(sdp, "a=channeloffset=", 0));
                        }

                        if(peer_found_via_cookie)
                        {
                            sidx = PEER_INDEX(peer_found_via_cookie);
                            printf("cookie idx for peer: %d\n", sidx);
                        }
                        else
                        {
                           sidx = sidx_offset;
                           while(peers[sidx].alive && sidx < MAX_PEERS) sidx++;
                           if(sidx >= MAX_PEERS) { free(sdp); goto response_override; }

                           peer_init(&peers[sidx], sidx);
                        }

                        sprintf(tmp, "webserver: new SDP for peer %d\n", sidx);
                        chatlog_append(tmp);

                        // mark -- wait for peer to be initialized
                        strcpy(tmp, str_read_unsafe(sdp, "a=myname=", 0));
                        peers[sidx].alive = 1;
                        peers[sidx].restart_needed = 1;

                        while(!peers[sidx].restart_done) usleep(10000);
                        peers[sidx].alive = 1;
                        peers[sidx].time_pkt_last = time(NULL);
                        strcpy(peers[sidx].name, tmp);

                        strncpy(peers[sidx].sdp.answer, sdp, sizeof(peers[sidx].sdp.answer));

                        strcpy(peers[sidx].stun_ice.ufrag_answer, sdp_read(sdp, "a=ice-ufrag:"));
                        sprintf(peers[sidx].stun_ice.ufrag_offer, "%s", "aaaaaaaa");

                        webserver.peer_index_sdp_last = sidx;

                        printf("peer restarted (stun_ice.uname:%s:%s)\n", peers[sidx].stun_ice.ufrag_answer, peers[sidx].stun_ice.ufrag_offer);

                        memset(user_fragment, 0, sizeof(user_fragment));

                        if(use_user_fragment_prefix)
                        {
                            char answer_ufrag[64];
                            const char* user_fragment_tag = "a=ice-ufrag:";
                            char* panswer_ufrag = strstr(sdp, user_fragment_tag);
                            if(panswer_ufrag)
                            {
                                panswer_ufrag += strlen(user_fragment_tag);
                                str_read(panswer_ufrag, answer_ufrag, ";\r\n", sizeof(answer_ufrag));
                            }
                            else
                            {
                                free(sdp);
                                goto response_override;
                            }

                            char* offer_ufrag = get_offer_sdp_idx((char*) user_fragment_tag, 0);
                            offer_ufrag += strlen(user_fragment_tag);

                            sprintf(user_fragment, "%s:%s", offer_ufrag, answer_ufrag);
                            printf("user_fragment:%s\n", user_fragment);
                        }

                        //sdp_prefix_set(user_fragment);

                        sprintf(tmp_filename, "%s%s", user_fragment, FILENAME_SDP_ANSWER);
                        printf("tmp_filename for POST: %s\n", tmp_filename);
                        file_write(sdp, strlen(sdp), tmp_filename);

                        {
                            file_buf = file_read("sdp_offer.txt", &file_buf_len);
                            if(file_buf_len > 0) {
                                strcpy(peers[sidx].sdp.offer, file_buf);
                            }
                        }

                        peers[sidx].restart_needed = 0;

                        free(sdp);

                        free(response);
                        response = strdup(page_buf_sdp_uploaded);
                        content_type = content_type_html;
                        goto response_override;
                    }
                    else if(strcmp(purl, "/chatmsg") == 0)
                    {
                        char *pchatmsg = pbody;

                        if(strncmp(pchatmsg, "msg=", 4)==0) pchatmsg += 4;
                        pchatmsg = sdp_decode(strdup(pchatmsg));
                      
                        chatlog_append(pchatmsg);
                       
                        free(response);
                        response = strdup(page_buf_redirect_back);
                        content_type = content_type_html;
                        goto response_override;
                    }
                    else if(strcmp(purl, "/subscribe") == 0)
                    {
                        peer_session_t* ps = peer_found_via_cookie;
                        unsigned int newChannel = 0;
                        char *p = pbody;
                        const char* subscribeKey = "subscriptionID=";
                        if(ps && strncmp(p, subscribeKey, strlen(subscribeKey)) == 0)
                        {
                            p += strlen(subscribeKey);
                            sscanf(p, "%u", &newChannel);
                            if(newChannel >= 0 && newChannel <= MAX_PEERS) {
                                ps->subscriptionID = peers[newChannel].id;
                            }
                        } 
                    }

                    free(response);
                    response = strdup(page_buf_redirect_subscribe);
                    content_type = content_type_html;
                }

                response_override:
                if(response && !timed_out)
                {
                    char *hdr = ok_hdr;
                    char clen_hdr[256];

                    r = send(sock, hdr, strlen(hdr), flags);
                    r = send(sock, cookie_hdr, strlen(cookie_hdr), flags);
                    sprintf(clen_hdr, "Content-Length: %lu\r\n", response_binary? file_buf_len: strlen(response));
                    r = send(sock, clen_hdr, strlen(clen_hdr), flags);
                    r = send(sock, content_type, strlen(content_type), flags);
                    r = send(sock, response, response_binary? file_buf_len: strlen(response), flags);
                }

                if(do_shutdown)
                {
                    /* indicate to close cxn when done */
                    shutdown(sock, SHUT_WR);
                }
    
                if(response) free(response);
            }

            if(!do_shutdown) continue;

            close(sock);
        }
    } while(0);

    free(args);

    printf("%s:%d exiting\n", __func__, __LINE__);
    return NULL;
}

void webserver_init()
{
    webserver.peer_index_sdp_last = -1;
    webserver.running = 1;
}

void*
webserver_accept_worker(void* p)
{    
    int backlog = 5;
    pthread_t thread;

    thread_init();

    int sock_web = bindsocket(webserver.inip, strToInt(get_config("webserver_port=")), 1);

    printf("%s:%d starting (sock_web=%d)\n", __func__, __LINE__, sock_web);

    if(sock_web >= 0)
    {
        int r = listen(sock_web, backlog);
    }
    
    while(webserver.running && sock_web >= 0)
    {
        struct sockaddr sa;
        socklen_t sa_len;
        int sock;
        int flags = 0;

        memset(&sa, 0, sizeof(sa));
        sa_len = sizeof(sa);

        sock = accept(sock_web, (struct sockaddr*) &sa, &sa_len);
        if(sock >= 0)
        {
            webserver_worker_args* args = (webserver_worker_args*) malloc(sizeof(webserver_worker_args));
            if(args)
            {
                args->ws_thread = 0;
                args->sock = sock;

                pthread_create(&thread, NULL, webserver_worker, args);
            }
        }
    }

    printf("%s:%d exiting\n", __func__, __LINE__);
}



#endif
