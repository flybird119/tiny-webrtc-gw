var divRoom = null;

var vidChildW = 128;
var vidChildH = 96;

var divVideoHTML = "" + 
"<div id=\"div_$USERNAME_child\">" +
"<td>" +
"<table id=\"table_$USERNAME\" border=0>"+
    "<tr><td>" +
        "<video controls autoplay class='videoChild' id=\"video_$USERNAME_remote\" width="+vidChildW.toString()+" height="+vidChildH.toString()+"></video>" +
    "</td>"+
    "<td>" +
        "<button id=\"btn_$USERNAME_1\" onClick=\"connectVideo(document.getElementById('video_$USERNAME_remote'), joinSDP(), true, '$USERNAME')\">show</button>" +
        "<button id=\"btn_$USERNAME_2\" onClick=\"onBtnMakePresent(this, '$USERNAME');\">presenter-ify</button>" +
        "<button id=\"btn_$USERNAME_3\" onClick=\"onBtnMute(this, '$USERNAME');\">stfu</button>" +
        "<button id=\"btn_$USERNAME_4\" onClick=\"onBtnClose(this, '$USERNAME');\">bye</button>" +
        "<input type=\"checkbox\" id=\"check_$USERNAME\" hidden=true>" +
    "</td></tr>"+
"</table>" +
"</td>" +
"<img hidden=true src='logo.jpg' onLoad='if(addUserLoad) addUserLoad(\"$USERNAME\"); this.hidden=true;'>" +
"</div>" +
"";
var addUserHTML = "<td><table border=1 style='margin:0; padding:0; display:inline-block; border-collapse:collapse;'><tr><td><img src='newUser.jpg'></td></tr><tr><td><button onClick='onBtnAddUser(\"\");'>Add User</button></td></tr></table></td>";
var winPopup = null;
var roomPopup = null;
var winPopupSdp = null;
var winPopupVideoTarget = null;
var winPopupRemoteConnection = null;
var videoConnectionTable = {}
var localStream = null;
var audioSourceN = 0;
var videoSourceN = 0;
var audioSource = null;
var videoSource = null;
var videoSourceLabel = null;
var audioSourceLabel = null;

var localVideo = document.getElementById("localVideo");
var vidElemPrevConnection = null;
var vidPresenter = null;

var joinPopupLast = {connection:null, userName:null, recvOnlyChecked:null, stream:null};

var stoppedStreamLast = null;
var userCounter = 1;
var roomTable = '' + addUserHTML;
var roomTableCols = 1;

var controlHeight = 40;
var mainDivW = 800;
var mainDivH = 600;
var mainDivX = 10;
var mainDivY = 10;

var vidChildX;
var vidChildY;

var addUserLoad = function(name) {
    var check = document.getElementById("check_"+name);
    if(!check.checked) {
        resizeObjectWithID("table_"+name, vidChildX, vidChildY, vidChildW+20, vidChildH+controlHeight);
        vidChildX += vidChildW + 100;
        if(vidChildX >= mainDivW) { 
            vidChildY += vidChildH+20;
            vidChildX = mainDivX;
        }
        check.checked = true;
    }
}

function vidChildInit() {
    vidChildX = mainDivX;
    vidChildY = mainDivY + mainDivH;
}

function getMedia() {
    navigator.mediaDevices.enumerateDevices().then(
        function(sourceInfos) {
            var ai = 0;
            var vi = 0;
            for(var i = 0; i < sourceInfos.length; i++) {
                console.log('mediaDevices('+sourceInfos[i].kind+')['+i+']: ' + sourceInfos[i].label);
                if(sourceInfos[i].kind == 'audio') {
                    if(audioSourceN == ai) {
                        audioSource = sourceInfos[i].id;
                        audioSourceLabel = sourceInfos[i].label;
                    }
                    ai++;
                }
                else if(sourceInfos[i].kind == 'video') {
                    if(videoSourceN == vi) {
                        videoSource = sourceInfos[i].id;
                        videoSourceLabel = sourceInfos[i].label;
                    }
                    vi++;
                }
            }

            var constraints = {
                audio: {
                    optional: [{sourceId: audioSource}]
                },
                video: {
                    optional: [{sourceId: videoSource}]
                }
            };

            getUserMedia(
                constraints,
                function (s) {
                    localStream = s;
                    attachMediaStream(localVideo, localStream);
                }
                ,
                function(e) {
                    alert('get media failed\nmaybe try https?\ncamera/mic enabled?\n\n(You can still watch other users)');
                }
            );
        }
    );
}

function broadcastOnLoad() {

    if(navigator.userAgent.indexOf('iPhone') >= 0 || navigator.userAgent.indexOf('iPad') >= 0) {
        if (confirm('browser not supported, visit the github project for help (iOS?))')) {
            window.location = 'https://github.com/justinb01981/tiny-webrtc-gw/wiki';
        }
    }

    if(document.cookie == '') {
        location = 'login.html';
    }
    divRoom = document.getElementById("roomDivCursor");

    mainDivW = (document.body.clientWidth / 100) * 80;
    mainDivH = (document.body.clientHeight / 100) * 70;

    //resizeObjectWithID("videoMain", mainDivX, mainDivY, (mainDivW/100)*60, (mainDivH/100)*50);

    vidChildInit();

    //resizeObjectWithID("mainDiv", mainDivX, mainDivY, mainDivW, mainDivH);
    //resizeObjectWithID("mainDivTable", mainDivX, mainDivY, mainDivW, mainDivH);

    getMedia();

    if('%$URLARGUMENTS$%'.length > 0) {
        document.getElementById('addUserLinkDiv').hidden = false;
    }

    var userTotal = 0;
    /*
    i = 0;
    while(i < peerList.length) {
        onBtnAddUser(peerList[i].name);
        i++;
        userTotal++;
    }
    */

    /*
    while(userTotal < 1) {
        onBtnAddUser('');
        userTotal++;
    }
    */
    
    resizeObjectWithID("roomAddButtonDiv", mainDivX, vidChildY+vidChildH/2, 50, 50);

    document.getElementById('userName').value = myUsername;

    setLoggedIn();

    onLoadDone();
}

function setLoggedIn() {
    var h = document.getElementById('login');
    if(myUsername == 'nobody') {
        h = document.getElementById('logout');
        document.getElementById('userName').style = 'display:none';
    }
    h.style = 'display:none;';
}

function logout() {
    location = 'logout.html';
}

function removeCookie() {
    document.cookie = "authCookieJS12242016=%$AUTHCOOKIE$%; expires=Thu, 01 Jan 1970 00:00:01 GMT; path=/";
    alert(myUsername + ' logged out...');
}

function macroHelper(a, b, c) {
    v = a;
    while(1) {
        var o = v;
        v = v.replace(b, c);
        if(o == v) break;
    }
    return v;
}

function openChat() {
    roomPopup = window.open('chat.html', 'room', 'width=300,height=300');
}
function rebootLocalVideo(vidElem) {
    if(vidElem.style.cssText.indexOf('none') >= 0) {
        vidElem.style.cssText = '';
    } else {
        vidElem.style.cssText = 'display:none;';
    }
}

function joinPopupClose(connection, userName, recvOnlyChecked) {
    joinPopupLast.connection = connection;
    joinPopupLast.userName = userName;
    joinPopupLast.recvOnlyChecked = recvOnlyChecked;
    joinPopupLast.stream = connection.getRemoteStreams()[0];

    winPopupRemoteConnection = connection;
    var w = winPopupVideoTarget.width;
    var h = winPopupVideoTarget.height;
    attachMediaStream(winPopupVideoTarget, winPopupRemoteConnection.getRemoteStreams()[0]);
    winPopupVideoTarget.width=w;
    winPopupVideoTarget.height=h;

    videoConnectionTable[winPopupVideoTarget.id] = winPopupRemoteConnection;

    var proto = document.location.protocol;
    var hostName = document.location.hostname;
    var hostPort = document.location.port;
}

function joinPopupOnLoadBroadcast() {
    var user = document.getElementById('userName').value;
    winPopup.document.theform.my_name.value = user;
    winPopup.document.theform.peerstream_recv.value = user;

    joinPopupOnLoad2();
}

function joinPopupOnLoadRecvOnly() {
    var user = 'watch' + Math.floor((Math.random() * 1000));
    winPopup.document.theform.my_name.value = user;
    winPopup.document.theform.peerstream_recv.value = user;
    joinPopupOnLoad2();
}

function joinPopupOnLoad2() {
    winPopup.document.theform.offersdp.value = winPopupSdp;
    winPopup.document.theform.answersdp.value = '';
    winPopup.localStream = localStream;
    winPopup.remoteVideo = winPopupVideoTarget;

    winPopup.closeHandler = joinPopupClose;
    winPopup.remoteConnection = new RTCPeerConnection(remoteConnectionStunConfig);
}



function disconnectVideo(vidElem) {
  if(videoConnectionTable[vidElem.id] != null)
  {
     var conn = videoConnectionTable[vidElem.id];
     if(conn.signalingState != 'closed') conn.close();
     videoConnectionTable[vidElem.id] = null;
  }
}

function connectVideo(videoElem, videoSDPOffer, recvOnly, watchUser) {
  var popupOnLoad = joinPopupOnLoadBroadcast;

  disconnectVideo(videoElem);

  if (recvOnly) popupOnLoad = joinPopupOnLoadRecvOnly;

  winPopupSdp = videoSDPOffer;
  winPopupVideoTarget = videoElem;
  winPopup = rtcPopupCreate(popupOnLoad, joinPopupClose, recvOnly, watchUser);

  if(!winPopup) alert('popup blocked?');
  //else window.onfocus = function(){ winPopup.close(); }
}

function onBtnAddUser(userName) {
    resizeObjectWithID("roomAddButtonDiv", mainDivX+(userCounter*(vidChildW+100)), vidChildY+vidChildH/2, 50, 50);
    vidChildInit();

    if(userName.length == 0) userName = userCounter.toString();
    var val = macroHelper(divVideoHTML, '$USERNAME', userName);
    roomTable += val;
    roomTableCols++;
    var divRoomCurs = document.getElementById("roomDivCursor");
    divRoomCurs.innerHTML = divRoomCurs.innerHTML + val;
    userCounter++;
}
function onBtnUsersExpand() {
    var d = document.getElementById('roomAddButtonDiv');
    if(d.style.cssText.indexOf('none') >= 0) {
        d.style.cssText = '';
    }
    else {
        d.style.cssText = 'display:none;';
    }
    resizeObjectWithID("roomAddButtonDiv", mainDivX+(userCounter*(vidChildW+100)), vidChildY+vidChildH/2, 50, 50);
}
function onBtnMute(btn, userName) {
    var vidSrc = document.getElementById('video_'+userName+'_remote');
    vidSrc.muted = !vidSrc.muted;
    if(vidPresenter == vidSrc) {
        document.getElementById('videoMain').muted = vidSrc.muted;
    }
}
function onBtnClose(btn, userName) {
    var vidSrc = document.getElementById('video_'+userName+'_remote');
    vidSrc.pause();
    var vidMain = document.getElementById('videoMain');
    //reattachMediaStream(vidSrc, null);
    if(vidPresenter == vidSrc) {
    vidSrc.pause();
        vidMain.pause();
        //reattachMediaStream(vidMain, null);
    }

    var d = document.getElementById('div_'+userName+'_child');
    d.style.cssText = 'display:none;';
}
var divPresenterClientWidth = 0;
var divPresenterClientHeight = 0;
function onBtnMakePresent(btn, userName) {
    var vid = document.getElementById('videoMain');
    var vidSrc = document.getElementById('video_'+userName+'_remote');
    reattachMediaStream(vid, vidSrc);
    vid.play();
    vidPresenter = vidSrc;

    var divPresenter = document.getElementById('thPresenter');
    if(divPresenterClientWidth == 0) divPresenterClientWidth = divPresenter.clientWidth;
    if(divPresenterClientHeight == 0) divPresenterClientHeight = divPresenter.clientHeight;
    vid.width = divPresenterClientWidth;
    vid.height = divPresenterClientHeight;

    //resizeObjectWithID("videoMain", mainDivX, mainDivY, (mainDivW/100)*60, (mainDivH/100)*50);
}
function mainVideoEmbiggen(pct) { 
}
function stopSending() {
    s = joinPopupLast.connection.getLocalStreams()[0]
    if (s) {
        stoppedStreamLast = s
        joinPopupLast.connection.removeStream(s)
    }
    else {
        joinPopupLast.connection.addStream(stoppedStreamLast)
    }
}

function channelPost() {
    document.channelForm.submit();
}
