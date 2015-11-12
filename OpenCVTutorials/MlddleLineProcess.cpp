//
//  MlddleLineProcess.cpp
//  OpenCVTutorials
//
//  Created by PeterLiu on 11/8/15.
//  Copyright © 2015 PeterLiu. All rights reserved.
//

#include "MlddleLineProcess.hpp"


/* Init the center path line */
CenterLineElement* initCenterPath()
{
    /**
        1. Load the image file.
     */
    // Load the image file
    Mat source = loadImageFile("/Users/peterliu/Documents/openDevelopment/one.jpg");
    imshow("source", source);
    
    /**
        2. Preprocess the image.
     */
    // Gray the image
    cvtColor(source, source, CV_BGR2GRAY);
    threshold(source, source, 128, 255, CV_THRESH_BINARY);
    Mat source_copy;
    source.copyTo(source_copy);
    // Change color---thinning need!
    bitwise_not(source, source);
    
    /**
        3. Get the skeletion of image.
     */
    
    Mat thinning;
    thinningImage(source, thinning);
    
    // Get the center line coordinates.
    int thin_rows = thinning.rows;
    int thin_cols = thinning.cols;
    
    CenterLineElement centerPaths[thin_cols];
    int pathIndex = 0;
    
    for (int x = 0; x < thin_cols; x++) {
        for (int y = 0; y < thin_rows; y++) {
            //
            Scalar color = thinning.at<uchar>(Point(x,y));
            if (color.val[0] > 254) {
                // Save the coordinates.
                centerPaths[pathIndex].x = x;
                centerPaths[pathIndex].y = y;
                // ra and rb
                // alpha
                pathIndex++;
            }
        }
    }
    
    /**
        4. Add the start and end segment line to the center line path.
     */
    // Get the start point.
    int leftPointX = 0;
    int leftPointY = 0;
    
    int rows = source_copy.rows;
    int cols = source_copy.cols;
    
    // Left start point
    for (int r = 0; r < rows; r++) {
        bool isEnd = false;
        for (int c = 0; c < cols; c++) {
            // left
            Scalar color = source_copy.at<uchar>(c,r);
            if (color.val[0] < 254) {
                leftPointX = r;
                leftPointY = c;
                isEnd = true;
                break;
            }
        }
        if (isEnd) {
            break;
        }
    }
    
    cout << "leftpoint:" << leftPointX << ":" << leftPointY << endl;
    
    // Left point to start point line.
    int leftSegmentLen = abs(leftPointX - centerPaths[0].x) > abs(leftPointY - centerPaths[0].y) ? abs(leftPointX - centerPaths[0].x) : abs(leftPointY - centerPaths[0].y);
    CenterLineElement leftsegment[leftSegmentLen];
    for (int x = leftPointX; x < centerPaths[0].x; x++) {
        int y2 = centerPaths[0].y;
        int x2 = centerPaths[0].x;
        int y = (x - leftPointX) / (x2 - leftPointX) * (y2 - leftPointY) + leftPointY;
        leftsegment[x - leftPointX].x = x;
        leftsegment[x - leftPointX].y = y;
    }
    
    for (int i = 0; i < leftSegmentLen; i++) {
        cout << leftsegment[i].x << ":" << leftsegment[i].y << endl;
    }
    
    

    
    /* Calculate the center line length. */
    int centerLength = 0;
    
    for (int i = 0; i < thin_cols; i++) {
        // print the center line coordinations
        if (centerPaths[i].x <= 1.0 || centerPaths[i].x > 300) {
            break;
        }
        centerLength++;
    }
    cout<< "Length:" << centerLength << endl;
    
    
    /* Calculate the alpha */
    for (int index = 0; index < centerLength; index++) {
        if (index == 0 || index == centerLength-1) {
            centerPaths[index].alpha = 0.0;
        }else {
            if (centerPaths[index].x - centerPaths[index-1].x != 0) {
                centerPaths[index].alpha = atan((centerPaths[index].y - centerPaths[index-1].y)/(centerPaths[index].x - centerPaths[index-1].x));
            }
        }
    }
    
    //Start point and end point, to keep the continuity.
    centerPaths[0].alpha = centerPaths[1].alpha;
    centerPaths[centerLength-1].alpha = centerPaths[centerLength - 2].alpha;
    
    // Remove noises too small == 0
    for (int i = 0; i < centerLength; i++) {
        if (centerPaths[i].alpha < 0.01) {
            centerPaths[i].alpha = 0.0;
        }
    }
    
    /* Calculate the Ra and Rb. */
    
    for (int i = 1; i < centerLength; i++) {
        //
        int x_o = centerPaths[i].x;
        int y_o = centerPaths[i].y;
        int offset = 1;
        // Positive
        int x = x_o + offset;
        int y;
        if (centerPaths[i].alpha != 0.0) {
            y = (x_o - x) / tan(centerPaths[i].alpha) + y_o;
        }else {
            y = y_o + offset;
        }
        Scalar color = source_copy.at<uchar>(Point(x, y));
        while (color.val[0] < 254) {
            x += offset;
            if (centerPaths[i].alpha != 0.0) {
                y = (x_o - x) / tan(centerPaths[i].alpha) + y_o;
            }else{
                y += offset;
            }
            color = source_copy.at<uchar>(Point(x, y));
        }
        int _x1 = x;
        int _y1 = y;
        
        //Negative
        x = x_o - offset;
        if (centerPaths[i].alpha != 0.0) {
            y = (x_o - x) / tan(centerPaths[i].alpha) + y_o;
        }else {
            y = y_o + offset;
        }
        
        color = source_copy.at<uchar>(Point(x,y));
        while (color.val[0] < 254) {
            x -= offset;
            if (centerPaths[i].alpha != 0.0) {
                y = (x_o - x) / tan(centerPaths[i].alpha) + y_o;
            }else{
                // Vertical coordinate
                y -= offset;
            }
            color = source_copy.at<uchar>(Point(x, y));
        }
        
        int _x2 = x;
        int _y2 = y;
        
        float tmpRb1 = sqrt((_y1 - y_o)*(_y1 - y_o) + (_x1 - x_o) * (_x1 - x_o));
        float tmpRb2 = sqrt((y_o - _y2) * (y_o - _y2) + (x_o - _x2) * (x_o - _x2));
        // Average of temp rb
        float rb = ( tmpRb1 + tmpRb2 ) / 2;
        
        centerPaths[i].rb = rb;
        centerPaths[i].ra = 5.0;
    }
    
    // start point ra rb;
    
    // Remove noise of ra / rb ，too small is zero
    for (int i = 0; i < centerLength; i++ ) {
        if (centerPaths[i].rb < 0.1) {
            centerPaths[i].rb = 0.0;
        }
    }
    
    
    for (int i = 0; i < centerLength; i++) {
        // print the center line coordinations
        cout << "(" << centerPaths[i].x << "," << centerPaths[i].y << ")" << "- " << centerPaths[i].alpha << "--" << centerPaths[i].rb <<endl;
    }
    return centerPaths;
}


/* Middle line or Center line */
void middleLine()
{
    int pathLen; // Path length
    
    //Read image
    Mat source = loadImageFile(imageSrc);
    
    // Gray the image
    cvtColor(source, source, CV_BGR2GRAY);
    threshold(source, source, 200, 255, CV_THRESH_BINARY);
    
    imshow("source", source);
    
    // Thinning function
//    Mat thinning;
//    thinningImage(source, thinning);
    
//    imshow("Thinning ", thinning);
    
    cout << source.channels() << endl;
    // Get the max length from left to right.
    
    int leftPointX = 0;
    int rigthPointX = 0;
    
    int rows = source.rows;
    int cols = source.cols;
    
    cout << rows << endl;
    cout << cols << endl;
    // Left start point
    for (int r = 0; r < rows; r++) {
        bool isEnd = false;
        for (int c = 0; c < cols; c++) {
            // left
            Scalar color = source.at<uchar>(c,r);
            if (color.val[0] < 254) {
                leftPointX = r;
                isEnd = true;
                break;
            }
        }
        if (isEnd) {
            break;
        }
    }
    
    // Right end point
    for (int r = rows-1; r >0 ; r--) {
        bool isEnd = false;
        for (int c = 0; c < cols; c++) {
            //Right
            Scalar color = source.at<uchar>(c,r);
            if (color.val[0] < 254) {
                rigthPointX = r;
                isEnd = true;
                break;
            }
        }
        if (isEnd) {
            break;
        }
    }
        
    cout << "left:" << leftPointX << endl;
    cout << "right:" << rigthPointX << endl;
    
    pathLen = rigthPointX - leftPointX;
    cout << "len:" << pathLen << endl;
    
    if (pathLen == 0) {
        return;
    }
    
    // Calculate the middle line of image.
    CenterLineElement paths[pathLen];
    
    for (int x = leftPointX; x < rigthPointX; x++) {
        // Get the middle line.
        int minCol = 0;
        int maxCol = 0;
        for (int y = 0; y < rows; y++) {
            //
            Scalar color = source.at<uchar>(y,x);
            // The black point.
            if (color.val[0] < 254) {
                // min col
                minCol = y;
                break;
            }
        }
        
        //Max col
        for (int y = minCol+1; y < rows; y++) {
            //
            Scalar color = source.at<uchar>(y,x);
            // The black point.
            if (color.val[0] >= 254) {
                // min col
                maxCol = y-1;
                break;
            }
        }
        //Path element
        int middleCol = minCol + (maxCol - minCol + 1) / 2;
        paths[x-leftPointX].y = x;
        paths[x-leftPointX].x = middleCol;
        paths[x-leftPointX].ra = ellipseRA;
        paths[x-leftPointX].rb = (maxCol - minCol + 1) / 2;
        
        // Calculate the alpha value
        if (x == leftPointX || x == rigthPointX-1) {
            paths[x-leftPointX].alpha = 0;
        }else {
            paths[x-leftPointX].alpha = atan((x - paths[x-leftPointX-1].y)/(middleCol - paths[x-leftPointX-1].x));
        }
        
        cout << paths[x-leftPointX].x << ":" << paths[x-leftPointX].y << "--" << paths[x-leftPointX].ra << ":" << paths[x-leftPointX].rb << endl;
        
    }
    //Modify the start and end alpha
    paths[0].alpha = paths[1].alpha;
    paths[rigthPointX-leftPointX-1].alpha = paths[rigthPointX-leftPointX-2].alpha;
    
    // Write to file
    FILE *ofp = fopen(pathTxt.c_str(), "w");
    
    if (ofp == NULL) {
        cout << "Open file error!" << endl;
    }
    
    for (int i = 0; i < pathLen; i++) {
        // writh file
        fprintf(ofp, "%f %f %f %f %f\n",paths[i].y,paths[i].x,paths[i].ra,paths[i].rb,paths[i].alpha);
    }
    fclose(ofp);

}

/* Load image file*/
Mat loadImageFile(String path)
{
    Mat source = imread(path);
    
    if (source.empty()) {
        cout << "Load file error!" << endl;
    }
    return source;
}
