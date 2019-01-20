# Logo-Recognition-App

## Environment
  * Win10
  * Android studio
  * Qt 5.2.1
  
## Program
  * [Camera2Basic] (https://github.com/merukoo0507/Logo-Recognition-App/blob/master/Camera2Basic/Application/src/main/java/com/example/android/camera2basic/Camera2BasicFragment.java)
    * The android app for take a photo, and send to Java server.
  * [RcvImgServer] (https://github.com/merukoo0507/Logo-Recognition-App/blob/master/RcvImgServer/JServer.java)
    * Receiving the photo from app, and notifying C server to do image recognition.
  * [LogoRecognitionServer] (https://github.com/merukoo0507/Logo-Recognition-App/blob/master/LogoRecognitionServer/main.cpp)
    * Do Image recognition and send the result to Java server.
    
## Flow
  1. run Cserver -> run Java server -> install Camera2Basic(App)
  2. run Camera2Basic(App) -> take a photo -> send 2 Java server -> notify C server
