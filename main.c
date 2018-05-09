#include "opencv2/video/tracking.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc_c.h"
#include <time.h>
#include <stdio.h>
#include <ctype.h>

char buff[255];

char* filename();
char* read();
void addLogging(char* add);
void newFile();
char* time_stamp();
void incrementCounter();

// various tracking parameters (in seconds)
const double MHI_DURATION = 1;
const double MAX_TIME_DELTA = 0.5;
const double MIN_TIME_DELTA = 0.05;
const int LOG_UPDATE_TIME = 5;

// number of cyclic frame buffer used for motion detection
// (should, probably, depend on FPS)
const int N = 4;

// ring image buffer
IplImage **buf = 0;
int last = 0;

// temporary images
IplImage *mhi = 0; // MHI
IplImage *orient = 0; // orientation
IplImage *mask = 0; // valid orientation maskz
IplImage *segmask = 0; // motion segmentation map
CvMemStorage* storage = 0; // temporary storage

// parameters:
//  img - input video frame
//  dst - resultant motion picture
//  args - optional parameters

int movement = 0;
int nonMovment = 0;
int startTime = 0;

char* filename() {
    time_t t = time(NULL);
    struct tm tm = *localtime(&t);

    // prefix
    const char *name = "log";

    // get month
    const char * months[12] = {"January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
    const char* mon = months[tm.tm_mon];

    // get year
    char year[15];
    sprintf(year, "%d", tm.tm_year + 1900);

    //fileextension
    const char *extension = ".txt";

    char* filename_with_extension;
    filename_with_extension = malloc(strlen(name)+1+4); /* make space for the new string (should check the return value ...) */
    strcpy(filename_with_extension, name); /* copy name into the new var */
    strcat(filename_with_extension, year); /* copy year into the new var */
    strcat(filename_with_extension, mon); /* copy mon into the new var */
    strcat(filename_with_extension, extension); /* add the extension */

    return filename_with_extension;
}

char* read(){

    FILE *fpR = fopen(filename(), "r+");

    if(fpR == NULL)
    {
        printf("Der opstod en fejl. - der bliver oprettet en ny log fil\n");
        do {
            newFile();
            fpR = fopen(filename(), "r+");
        } while (fpR == NULL);
    }
    // hiver data ud af fil
    fscanf(fpR, "%s", buff);
    printf("read from file : %s\n", buff);
    fclose(fpR);
    return buff;
}

void addLogging(char* add){
    FILE *fpAdd = fopen(filename(), "a+");

    incrementCounter();

    // printf("%s \n", add);

    size_t len = strlen(add) + strlen("\n");
    char *ret = (char*)calloc(len * sizeof(char) + 1, 1);

    fprintf(fpAdd, "%s", strcat(strcat(ret, add) , "\n"));

    //  printf("%s \n", add);

    free(ret);

    fclose(fpAdd);
}

void newFile(){
    FILE *fpW = fopen(filename(), "w");

    fseek(fpW, 0, SEEK_SET);

    fprintf(fpW, "0 \n");

    fclose(fpW);

    printf("Ny log fil oprettet.\n");
}

char* time_stamp(){ // modyfied vertion of from https://stackoverflow.com/questions/1444428/time-stamp-in-the-c-programming-language
    char *timestamp = (char *)malloc(sizeof(char) * 16);
    time_t ltime;
    ltime=time(NULL);
    struct tm *tm;
    tm=localtime(&ltime);

    sprintf(timestamp,"%04d-%02d-%02d %02d:%02d:%02d", tm->tm_year+1900, tm->tm_mon,
            tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    //printf(timestamp,"%04d-%02d-%02d %02d:%02d:%02d \n", tm->tm_year+1900, tm->tm_mon,
    //       tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

    return timestamp;
}

void incrementCounter(){
    FILE *fpR;
    char number[5];

    fpR = fopen(filename(), "r+");
    fseek(fpR, 0, SEEK_SET);
    fscanf(fpR,"%20[^\n]\n",number); // kan incrementere tal op til 20 cifre.
    fseek(fpR, 0, SEEK_SET);

   // int count = atoi(number);

    char *ptr;
   // long ret;

    long count = strtol(number, &ptr, 10); // base 10

    count++;

    //char str[20];

    sprintf(ptr, "%d", count);
    // Now str contains the integer as characters

    fprintf(fpR, ptr);
    fclose(fpR);
}

void checkMove(){
    if(movement > nonMovment){
        printf("movement at: %s \n", time_stamp());
        addLogging(time_stamp());
        CvCapture* capture =cvCreateCameraCapture(0);
        IplImage* frame = cvQueryFrame(capture);
            if(!frame){
                printf("Error. Cannot get the frame.");
            }
        CvFont base_font;

        CvScalar color = CV_RGB(255,0,0);

        cvInitFont( &base_font, CV_FONT_HERSHEY_SIMPLEX, 1.5, 1.5, 0, 1, 1);

        cvPutText(frame, time_stamp(), cvPoint( 20, 50 ), &base_font, color);

        char src[50];

        strcpy(src, time_stamp());
        strcat(src, ".jpg");

        cvSaveImage(src, frame, 0);

    } else{
        printf("No movement\n");
    }
}

static void  update_mhi( IplImage* img, IplImage* dst, int diff_threshold ) {
    double timestamp = (double)clock()/CLOCKS_PER_SEC; // get current time in seconds
    CvSize size = cvSize(img->width,img->height); // get current frame size
    int i, idx1 = last, idx2;
    IplImage* silh;
    CvSeq* seq;
    CvRect comp_rect;
    double count;
    double angle;
    CvPoint center;
    double magnitude;
    CvScalar color;

    // allocate images at the beginning or
    // reallocate them if the frame size is changed
    if( !mhi || mhi->width != size.width || mhi->height != size.height ) {
        if( buf == 0 ) {
            buf = (IplImage**)malloc(N*sizeof(buf[0]));
            memset( buf, 0, N*sizeof(buf[0]));
        }

        for( i = 0; i < N; i++ ) {
            cvReleaseImage( &buf[i] );
            buf[i] = cvCreateImage( size, IPL_DEPTH_8U, 1 );
            cvZero( buf[i] );
        }
        cvReleaseImage( &mhi );
        cvReleaseImage( &orient );
        cvReleaseImage( &segmask );
        cvReleaseImage( &mask );

        mhi = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        cvZero( mhi ); // clear MHI at the beginning
        orient = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        segmask = cvCreateImage( size, IPL_DEPTH_32F, 1 );
        mask = cvCreateImage( size, IPL_DEPTH_8U, 1 );
    }

    cvCvtColor( img, buf[last], CV_BGR2GRAY ); // convert frame to grayscale

    idx2 = (last + 1) % N; // index of (last - (N-1))th frame
    last = idx2;

    silh = buf[idx2];
    cvAbsDiff( buf[idx1], buf[idx2], silh ); // get difference between frames

    cvThreshold( silh, silh, diff_threshold, 1, CV_THRESH_BINARY ); // and threshold it
    cvUpdateMotionHistory( silh, mhi, timestamp, MHI_DURATION ); // update MHI

    // convert MHI to blue 8u image
    cvCvtScale( mhi, mask, 255./MHI_DURATION,
                (MHI_DURATION - timestamp)*255./MHI_DURATION );
    cvZero( dst );
    cvMerge( mask, 0, 0, 0, dst );

    // calculate motion gradient orientation and valid orientation mask
    cvCalcMotionGradient( mhi, mask, orient, MAX_TIME_DELTA, MIN_TIME_DELTA, 3 );

    if( !storage )
        storage = cvCreateMemStorage(0);
    else
        cvClearMemStorage(storage);

    // segment motion: get sequence of motion components
    // segmask is marked motion components map. It is not used further
    seq = cvSegmentMotion( mhi, segmask, storage, timestamp, MAX_TIME_DELTA );

    // iterate through the motion components,
    // One more iteration (i == -1) corresponds to the whole image (global motion)
    for( i = -1; i < seq->total; i++ ) {

        if( i < 0 ) { // case of the whole image
            comp_rect = cvRect( 0, 0, size.width, size.height );
            color = CV_RGB(255,255,255);
            magnitude = 100;
        }
        else { // i-th motion component
            comp_rect = ((CvConnectedComp*)cvGetSeqElem( seq, i ))->rect;
            if( comp_rect.width + comp_rect.height < 100 ) // reject very small components
                continue;
            color = CV_RGB(255,0,0);
            magnitude = 30;
        }

        // select component ROI
        cvSetImageROI( silh, comp_rect );
        cvSetImageROI( mhi, comp_rect );
        cvSetImageROI( orient, comp_rect );
        cvSetImageROI( mask, comp_rect );

        // calculate orientation
        angle = cvCalcGlobalOrientation( orient, mask, mhi, timestamp, MHI_DURATION);
        angle = 360.0 - angle;  // adjust for images with top-left origin

        count = cvNorm( silh, 0, CV_L1, 0 ); // calculate number of points within silhouette ROI

        cvResetImageROI( mhi );
        cvResetImageROI( orient );
        cvResetImageROI( mask );
        cvResetImageROI( silh );

        time_t t = time(NULL);
        struct tm tm = *localtime(&t);

        // check for the case of little motion
        if( count < comp_rect.width*comp_rect.height * 0.05 ) {
            nonMovment++;
        } else {
            movement++;
        }

        if(startTime == 0){
            startTime = t;
        }

        int now = t;
        int then = startTime;

        if(now >= then + LOG_UPDATE_TIME){
            checkMove();
            startTime = t;
            movement = 0;
            nonMovment = 0;
        }
    }
}



int main(int argc, char** argv) {
    read();
    IplImage* motion = 0;
    CvCapture* capture = 0;

    if( argc == 1 || (argc == 2 && strlen(argv[1]) == 1 && isdigit(argv[1][0])))
        capture = cvCaptureFromCAM( argc == 2 ? argv[1][0] - '0' : 0 );
    else if( argc == 2 )
        capture = cvCaptureFromFile( argv[1] );

    if( capture )
    {
        cvNamedWindow( "Motion", 1 );

        for(;;) {
            IplImage* image = cvQueryFrame( capture );
            if( !image )
                break;

            if( !motion ) {
                motion = cvCreateImage( cvSize(image->width,image->height), 8, 3 );
                cvZero( motion );
                motion->origin = image->origin;
            }

            update_mhi( image, motion, 30 );
            cvShowImage( "Motion", motion );

            if( cvWaitKey(10) >= 0 )
                break;
        }
        cvReleaseCapture( &capture );
        cvDestroyWindow( "Motion" );
    }

    return 0;
}

#ifdef _EiC
main(1,"motempl.c");
#endif