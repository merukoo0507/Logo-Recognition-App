#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#pragma comment(lib, "Ws2_32.lib")
#include <WS2tcpip.h>
#include <WinSock2.h>
#include "opencv2/features2d/features2d.hpp"
#include "opencv2/nonfree/nonfree.hpp"

#include <iostream>
#include <windows.h>
#include <string>
#include <sys/types.h>
#include <vector>
#include <dirent.h>
#include <errno.h>
#include<iostream>
#include<fstream>
#include <time.h>

using namespace cv;
using namespace std;

vector <string> classname;
vector <Mat> vctRefImage;
vector <int> vctIdxRef;
//Project dir, Normalized image dir
string pjtDir = "C:\\Merukoo\\Logo-Recognition-App\\";
string refDir = pjtDir + "LogoSrc";
string ans = "NoData";

//string testImagePath;
string testImagePath = pjtDir + "RcvImgServer\\test.jpg";
string dictionary_path = pjtDir + "dictionary.yml";

//for testing
string DrawImage = pjtDir + "Draw.jpg";

int getdir(string dir, vector<string> &files);
void className2Vct();
void buildDictionary();
void* CServer(void*);


void crossCheckMatching( Ptr<DescriptorMatcher>& descriptorMatcher,
                         const Mat& descriptors1, const Mat& descriptors2,
                         vector<DMatch>& filteredMatches12, int knn=1 )
{
    filteredMatches12.clear();
    vector<vector<DMatch> > matches12, matches21;
    descriptorMatcher->knnMatch( descriptors1, descriptors2, matches12, knn );
    descriptorMatcher->knnMatch( descriptors2, descriptors1, matches21, knn );
    for( size_t m = 0; m < matches12.size(); m++ )
    {
        bool findCrossCheck = false;
        for( size_t fk = 0; fk < matches12[m].size(); fk++ )
        {
            DMatch forward = matches12[m][fk];

            for( size_t bk = 0; bk < matches21[forward.trainIdx].size(); bk++ )
            {
                DMatch backward = matches21[forward.trainIdx][bk];
                if( backward.trainIdx == forward.queryIdx )
                {
                    filteredMatches12.push_back(forward);
                    findCrossCheck = true;
                    break;
                }
            }
            if( findCrossCheck ) break;
        }
    }
}


void DetectImageSetsKeypointDescriptor(vector <Mat> vctImg,
                                       vector < vector<KeyPoint> > &keypoints,
                                       vector < Mat > &descriptors,
                                       Ptr<FeatureDetector>& detector, Ptr<DescriptorExtractor>& descriptorExtractor);

double doIteration( const Mat& img1, Mat& img2,
                         vector<KeyPoint> &keypoints1, const Mat& descriptors1,
                         vector<KeyPoint> &keypoints2, const Mat& descriptors2,
                         Ptr<DescriptorMatcher>& descriptorMatcher,
                         double ransacReprojThreshold, BOWImgDescriptorExtractor &bowDE, bool show);
void BOOW();

int main()
{
    cout << "--------Cls2Vec--------" << endl;
    className2Vct();
    cout << "--------BuildDic--------" << endl;
    buildDictionary();
    cout << "--------Testing--------" << endl;
    //BOOW();

    cout << "--------CServer--------" << endl;
    pthread_t t; // 宣告 pthread 變數
    pthread_create(&t, NULL, CServer, NULL); // 建立子執行緒
    pthread_join(t, NULL); // 等待子執行緒執行完成
    return 0;
}

void* CServer(void*)
{
    int r;
    WSAData wsaData;
    WORD DLLVSERION;
    DLLVSERION = MAKEWORD(2, 1);//Winsocket-DLL 版本

                                //用 WSAStartup 開始 Winsocket-DLL
    r = WSAStartup(DLLVSERION, &wsaData);

    //宣告 socket 位址資訊(不同的通訊,有不同的位址資訊,所以會有不同的資料結構存放這些位址資訊)
    SOCKADDR_IN addr;
    int addrlen = sizeof(addr);

    //建立 socket
    SOCKET sListen; //listening for an incoming connection
    SOCKET sConnect; //operating if a connection was found

    //AF_INET：表示建立的 socket 屬於 internet family
    //SOCK_STREAM：表示建立的 socket 是 connection-oriented socket
    sConnect = socket(AF_INET, SOCK_STREAM, NULL);

    //設定位址資訊的資料
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(1234);

    //設定 Listen
    sListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);//SOMAXCONN: listening without any limit

                               //等待連線
    SOCKADDR_IN clinetAddr;
    while (true)
    {
        cout << "waiting..." << endl;

        if (sConnect = accept(sListen, (SOCKADDR*)&clinetAddr, &addrlen))
        {
            cout << "a connection was found" << endl;

            int bytes = 0;
            char recvbuf[256];
            memset(recvbuf, '\0', sizeof(recvbuf));

            while (true) {
                bytes = recv(sConnect, recvbuf, 256, 0);
                if (bytes > 0) {
                    cout << "recvbuf: " << recvbuf << endl;
                    testImagePath = recvbuf;
                    break;
                }
            }
            BOOW();

            //傳送訊息給 client 端
            const char * chr = ans.c_str();
            char *sendbuf = const_cast<char *>(chr);
            send(sConnect, sendbuf, (int)strlen(sendbuf), 0);
        }
    }

  pthread_exit(NULL); // 離開子執行緒
}

void BOOW()
{
    //====BOOW===========================================================================================
    Ptr<FeatureDetector> detector(new SurfFeatureDetector());;
    Ptr<DescriptorExtractor> descriptorExtractor(new SurfDescriptorExtractor);;
    Ptr<DescriptorMatcher> descriptorMatcher = DescriptorMatcher::create("FlannBased");//"BruteForce");
    vector < vector<KeyPoint> > refKeypoints(vctRefImage.size());
    vector < Mat > refDescriptors(vctRefImage.size());
    DetectImageSetsKeypointDescriptor(vctRefImage, refKeypoints, refDescriptors, detector, descriptorExtractor);

    //====DICTIONARY===========================================================================================
    Mat mDictionary;
    FileStorage fs(dictionary_path, FileStorage::READ);
    fs["vocabulary"] >> mDictionary;
    fs.release();

    Ptr<DescriptorMatcher> matcher(new FlannBasedMatcher);//create a nearest neighbor matcher
    Ptr<DescriptorExtractor> extractor(new SurfDescriptorExtractor);//create SURF descriptor extractor
    BOWImgDescriptorExtractor bowDE(extractor, matcher);//create BoW descriptor extractor
    bowDE.setVocabulary(mDictionary);


    int ransacReprojThreshold = 3;

    Mat testImage = imread(testImagePath, CV_LOAD_IMAGE_GRAYSCALE);
    if (testImage.empty())
    {
        cout << "No testImage." << endl;
        ans = "nodata#";
        return;
    }
//            else
//            {
//                if (testImage.rows < testImage.cols)
//                {
//                    float rr = 150.0 / testImage.rows;
//                    int ww = testImage.cols * rr;
//                    resize(testImage, testImage, Size(ww, 150));
//                }
//                else{
//                    float rr = 150.0 / testImage.cols;
//                    int hh = testImage.rows * rr;
//                    resize(testImage, testImage, Size(150, hh));
//                }

//    Rect rect(166,200,300,300);
//    testImage = testImage(rect);
//    imwrite(testImagePath2, testImage);

    vector<KeyPoint> testKeypoints;
    Mat testDescriptors;
    detector->detect(testImage, testKeypoints);
    descriptorExtractor->compute(testImage, testKeypoints, testDescriptors);

    float minDistance = 9e+9;
    int minIndex = 0;
    for (int j = 0; j < vctRefImage.size(); j++)
    {
        if (testKeypoints.size()==0)
        {
            ans = "nodata#";
            break;
        }
        double disValue = doIteration(testImage, vctRefImage[j],
                                      testKeypoints, testDescriptors,
                                      refKeypoints[j], refDescriptors[j],
                                      descriptorMatcher, ransacReprojThreshold, bowDE, false);
        if (disValue < minDistance)
        {
            minDistance = disValue;
            minIndex = j;
        }
    }
/*
    doIteration(testImage, vctRefImage[minIndex],
                testKeypoints, testDescriptors,
                refKeypoints[minIndex], refDescriptors[minIndex],
                descriptorMatcher, ransacReprojThreshold, bowDE, true);    //true
*/
    ans = classname[vctIdxRef[minIndex]]+"#";
    cout << "class:" << ans << endl;
    //endTime=clock();
    //runningTime = (double)(endTime-startTime)/CLOCKS_PER_SEC;
    //cout << runningTime << endl;
}

void DetectImageSetsKeypointDescriptor(vector <Mat> vctImg,
                                       vector < vector<KeyPoint> > &keypoints,
                                       vector < Mat > &descriptors,
                                       Ptr<FeatureDetector>& detector, Ptr<DescriptorExtractor>& descriptorExtractor)
{
    for (int i = 0; i < vctImg.size(); i++)
    {
        detector->detect(vctImg[i], keypoints[i]);
        descriptorExtractor->compute(vctImg[i], keypoints[i], descriptors[i]);
    }
}
double doIteration( const Mat& img1, Mat& img2,
                         vector<KeyPoint> &keypoints1, const Mat& descriptors1,
                         vector<KeyPoint> &keypoints2, const Mat& descriptors2,
                         Ptr<DescriptorMatcher>& descriptorMatcher,
                         double ransacReprojThreshold, BOWImgDescriptorExtractor &bowDE, bool show)
{
    //====Cross Check and filter==============================================================
    //決定那些配對的點座標位置是相近的, 只會把相似的特徵(有匹配到的特徵)拿去做BOW, 因為拍照的有許多雜訊
    vector<DMatch> filteredMatches;
    crossCheckMatching( descriptorMatcher, descriptors1, descriptors2, filteredMatches, 1 );

    if (filteredMatches.size() > 3)
    {
        //filterkeypoints1 與 filterkeypoints2就是剩下的特徵點
        //一開始的keypoints1與keypoints2就是原本一開始偵測到的, 中間的過程就是會刪掉不要的特最後剩下的就是filterkeypoints
        vector<int> queryIdxs(filteredMatches.size());
        vector<int> trainIdxs(filteredMatches.size());
        vector <KeyPoint> filterkeypoints1(filteredMatches.size());
        vector <KeyPoint> filterkeypoints2(filteredMatches.size());
        for (size_t i = 0; i < filteredMatches.size(); i++)
        {
            queryIdxs[i] = filteredMatches[i].queryIdx;//query = descriptors1
            trainIdxs[i] = filteredMatches[i].trainIdx;//train = descriptors2
            filterkeypoints1[i] = keypoints1[filteredMatches[i].queryIdx];
            filterkeypoints2[i] = keypoints2[filteredMatches[i].trainIdx];
        }

        //====Find homography (RANSAC) matrix======================================================
        vector<Point2f> points1;
        vector<Point2f> points2;
        KeyPoint::convert(keypoints1, points1, queryIdxs);//KeyPoint convert to Point2f
        KeyPoint::convert(keypoints2, points2, trainIdxs);//KeyPoint convert to Point2f

        Mat H12;
        if (ransacReprojThreshold >= 0)
        {
            H12 = findHomography(Mat(points1), Mat(points2), CV_RANSAC, ransacReprojThreshold);
        }

        //====決定那些配對的點座標位置是相近的=========================================================
        //相近的小於maxInlierDist
        Mat points1t;
        vector<char> matchesMask(filteredMatches.size(), 0);
        perspectiveTransform(Mat(points1), points1t, H12);

        double maxInlierDist = ransacReprojThreshold < 0 ? 3 : ransacReprojThreshold;

        for (int i1 = (points1.size() - 1); i1 >= 0; i1--)
        {
            double l2dist = norm(points2[i1] - points1t.at<Point2f>(i1, 0));
            if (l2dist <= maxInlierDist) // inlier
            {
                matchesMask[i1] = 1;

            }
            else
            {
                filterkeypoints1.erase(filterkeypoints1.begin() + i1);
                filterkeypoints2.erase(filterkeypoints2.begin() + i1);
            }

        }

        if (filterkeypoints1.size() > 0)
        {
            if (show)
            {
                // draw inliers
                Mat drawImg;
                drawMatches(img1, keypoints1, img2, keypoints2, filteredMatches, drawImg, CV_RGB(0, 255, 0), CV_RGB(0, 0, 255), matchesMask);

                //-----------------------------------------------------------------------------------------

                //====圖像的校正============================================================================
                Mat warpImg = img2.clone();
                warpPerspective(img1, warpImg, H12, warpImg.size());
                //			warpImg = warpImg.rowRange(0, img1.rows).colRange(0, img1.cols).clone();
                //-----------------------------------------------------------------------------------------
//                imshow("warpPerspective", warpImg);
//                imshow("correspondences", drawImg);
                imwrite(DrawImage, drawImg);
            }

            //====Computing BOW feature and distance===================================================
            Mat bowDescriptor1;
            Mat bowDescriptor2;

            bowDE.compute(img1, filterkeypoints1, bowDescriptor1);//extract SURF
            bowDE.compute(img2, filterkeypoints2, bowDescriptor2);//extract SURF
            return (norm(bowDescriptor1, bowDescriptor2, NORM_L2));
            //-----------------------------------------------------------------------------------------
        }
        else
            return 9e+9;
    }
    else
        return 9e+9;
}

void buildDictionary()
{
    Ptr<FeatureDetector> detector(new SurfFeatureDetector());;
    Ptr<DescriptorExtractor> descriptorExtractor(new SurfDescriptorExtractor);;
    Ptr<DescriptorMatcher> descriptorMatcher = DescriptorMatcher::create("FlannBased");//"BruteForce");
    vector < vector<KeyPoint> > refKeypoints(vctRefImage.size());
    vector < Mat > refDescriptors(vctRefImage.size());
    DetectImageSetsKeypointDescriptor(vctRefImage, refKeypoints, refDescriptors, detector, descriptorExtractor);
    Mat featuresUnclustered;

    for (int i = 0; i < refDescriptors.size(); i++)
    {
        featuresUnclustered.push_back(refDescriptors[i]);
    }
    int dictionarySize = 2500;
    TermCriteria tc(CV_TERMCRIT_ITER, 100, 0.001);
    int retries = 1;
    int flags = KMEANS_PP_CENTERS;
    BOWKMeansTrainer bowTrainer(dictionarySize, tc, retries, flags);
    Mat mDictionary = bowTrainer.cluster(featuresUnclustered);

    BOWImgDescriptorExtractor bowDE(descriptorExtractor, descriptorMatcher);//create BoW descriptor extractor
    bowDE.setVocabulary(mDictionary);
    FileStorage fs(dictionary_path, FileStorage::WRITE);
    fs << "vocabulary" << mDictionary;
}

int getdir(string dir, vector<string> &files)
{
    DIR *dp;//創立資料夾指標
    struct dirent *dirp;
    if((dp = opendir(dir.c_str())) == NULL){
        cout << "Error(" << errno << ") opening " << dir << endl;
        return errno;
    }
    while((dirp = readdir(dp)) != NULL){//如果dirent指標非空
        files.push_back(string(dirp->d_name));//將資料夾和檔案名放入vector
    }
    closedir(dp);//關閉資料夾指標
    return 0;
}

void className2Vct()
{
    vector<string> files;

    getdir(refDir, files);
    for (int i=2; i<files.size(); i++)
    {
        string s = refDir + "\\" + files[i];
        //char *loc = strstr(const_cast<char*>(files[i].c_str()), ".jpg");
        //if (loc != NULL)
        classname.push_back(files[i]);
    }
    //cout << "--------classname/final--------" << endl;
    files.clear();

    for (int c = 0; c < classname.size(); c++)
    {
        string s = refDir + "\\" + classname[c];
        getdir(s, files);
        for (int i = 0; i < files.size(); i++)
        {
            s = refDir + "\\" + classname[c] + "\\" + files[i];
            Mat img = imread(s, CV_LOAD_IMAGE_GRAYSCALE);
            if (img.empty())
            {
                //cout << "class image error." << endl;
                //exit(-1);
                continue;
            }
            vctRefImage.push_back(img);
            vctIdxRef.push_back(c);
        }
        files.clear();
    }
    //cout << "--------vctRefImage&vctIdxRef/final--------" << endl;
    //system("cls");
}
