/*                                                                              
 * Copyright (C) 2013 Leslie Zhai <zhaixiang@linuxdeepin.com> 
 *                                                                              
 * This program is free software: you can redistribute it and/or modify         
 * it under the terms of the GNU General Public License as published by         
 * the Free Software Foundation, either version 3 of the License, or            
 * any later version.                                                           
 *                                                                              
 * This program is distributed in the hope that it will be useful,              
 * but WITHOUT ANY WARRANTY; without even the implied warranty of               
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the                
 * GNU General Public License for more details.                                 
 *                                                                              
 * You should have received a copy of the GNU General Public License            
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.        
 */

#include <iostream>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "gesture_recognition.h"

namespace MTGR 
{
    GestureRecognition::GestureRecognition() {}
    
    GestureRecognition::~GestureRecognition() {}

    MT_GESTURE_CLASS GestureRecognition::predictSlot(std::vector<point_t> slotPoints) 
    {
        MT_GESTURE_CLASS predictSlotValue = NULL_GESTURE;
        int x1, y1;
        int x2, y2;
        int size = slotPoints.size();

        if (size < 2) 
            return predictSlotValue;

        x1 = slotPoints[0].x;
        y1 = slotPoints[0].y;
        x2 = slotPoints[size - 1].x;
        y2 = slotPoints[size - 1].y;

        if (abs(y1 - y2) < abs(x1 - x2)) {
            if (x1 < x2) {
#if DEBUG
                std::cout << "DEBUG: rightward" << std::endl;
#endif
                predictSlotValue = RIGHTWARD;
                return predictSlotValue;
            } else {
#if DEBUG
                std::cout << "DEBUG: leftward" << std::endl;
#endif
                predictSlotValue = LEFTWARD;
                return predictSlotValue;
            }
        } else {
            if (y1 < y2) {
#if DEBUG
                std::cout << "DEBUG: downward" << std::endl;
#endif
                predictSlotValue = DOWNWARD;
                return predictSlotValue;
            } else {
#if DEBUG
                std::cout << "DEBUG: upward" << std::endl;
#endif
                predictSlotValue = UPWARD;
                return predictSlotValue;
            }
        }

        return predictSlotValue;
    }

    MT_GESTURE_CLASS GestureRecognition::predictEllipse(std::vector<point_t> points1, 
                                                        std::vector<point_t> points2) 
    {
        MT_GESTURE_CLASS predictEllipseValue = CLOCKWISE;
        int y1max = 0;
        int y2max = 0;
        int i;

        for (i = 0; i < points1.size(); i++) {
            if (y1max < points1[i].y) 
                y1max = points1[i].y;
        }
        for (i = 0; i < points2.size(); i++) {                                     
            if (y2max < points2[i].y)                                              
                y2max = points2[i].y;                                              
        }
        MT_GESTURE_CLASS predictTempClass = predictSlot(y1max < y2max ? points1 : points2);
        if (predictTempClass == LEFTWARD || predictTempClass == DOWNWARD) 
            predictEllipseValue = ANTICLOCKWISE;

        return predictEllipseValue;
    }

    MT_GESTURE_CLASS GestureRecognition::predictSlot(std::vector<point_t> points1, 
                                                     std::vector<point_t> points2) 
    {
        MT_GESTURE_CLASS predictSlotValue = NULL_GESTURE;
        int x11, x12, y11, y12, x21, x22, y21, y22, c1, c2;
        int size;
        int loop;

        size = points1.size();
        if (size < 2) {
#if DEBUG
            std::cout << "DEBUG: points1 size < 2" << std::endl;
#endif
            return predictSlotValue;
        }

        x11 = points1[0].x;
        y11 = points1[0].y;
        x12 = points1[size - 1].x;
        y12 = points1[size - 1].y;

        size = points2.size();
        if (size < 5) { 
#if DEBUG
            std::cout << "DEBUG: points2 size < 5" << std::endl;
#endif
            return predictSlotValue;
        }

        x21 = points2[5].x; y21 = points2[5].y; 
        x22 = points2[size - 1].x; y22 = points2[size - 1].y;
        c1 = sqrt(pow(abs(x21 - x11), 2) + pow(abs(y21 - y11), 2));
        c2 = sqrt(pow(abs(x22 - x12), 2) + pow(abs(y22 - y12), 2));

        if (c1 < c2) {
#if DEBUG
            std::cout << "DEBUG: zoom out" << std::endl;
#endif 
            predictSlotValue = ZOOMOUT;
        } else {
#if DEBUG
            std::cout << "DEBUG: zoom in" << std::endl;
#endif
            predictSlotValue = ZOOMIN;
        }

        return predictSlotValue;
    }

    point_t GestureRecognition::getCentralPoint(std::vector<point_t> points1, 
                                                std::vector<point_t> points2, 
                                                bool & isEllipse) 
    {
        point_t cp;
        cp.x = -1;
        cp.y = -1;

        if (points1.size() == 0 || points2.size() == 0) 
            return cp;

        point_t point1 = points1[points1.size() - 1];
        point_t point2 = points2[points2.size() - 1];

        cp.x = abs(point1.x - point2.x) / 2 + std::min(point1.x, point2.x);
        cp.y = abs(point1.y - point2.y) / 2 + std::min(point1.y, point2.y);
#if DEBUG
        std::cout << "DEBUG: central point " << cp.x << ", " << cp.y << std::endl;
#endif
        if ((cp.x < points1[0].x && cp.x < point1.x && points2[0].x < cp.x && point2.x < cp.x) || 
            (cp.x > points1[0].x && cp.x > point1.x && points2[0].x > cp.x && point2.x > cp.x)) { 
#if DEBUG
            std::cout << "DEBUG: is not ellipse" << std::endl;
#endif
            isEllipse = false;
        } else {
#if DEBUG
            std::cout << "DEBUG: is ellipse" << std::endl;
#endif
            isEllipse = true;
        }

        return cp;
    }

    MT_GESTURE_CLASS GestureRecognition::predict(std::vector<sample_t> samples) 
    {
        MT_GESTURE_CLASS predictReturn = NULL_GESTURE;
        std::vector<point_t> * slotPoints = NULL;
        std::map<MT_GESTURE_CLASS, int> predictClass;
        std::set<int>::iterator slotIter;
        std::map<MT_GESTURE_CLASS, int>::iterator predictIter;
        bool isEllipse;
        int i;
        int j;
        
        for (i = 0; i < samples.size(); i++) 
            slots.insert(samples[i].slot);
#if DEBUG
        std::cout << "DEBUG: slots size " << slots.size() << std::endl;
#endif
        if (slots.size() < 1) {
            std::cout << "ERROR: slots size < 1" << std::endl;
            return predictReturn;
        }

        slotPoints = new std::vector<point_t>[slots.size() + 1];
        for (slotIter = slots.begin(); slotIter != slots.end(); slotIter++) {
            for (j = 0; j < samples.size(); j++) {
                if (*slotIter == samples[j].slot && 
                    *slotIter >= 0 && 
                    *slotIter < slots.size()) {
                    point_t pointTemp;
                    pointTemp.x = samples[j].point.x;
                    pointTemp.y = samples[j].point.y;
                    slotPoints[*slotIter].push_back(pointTemp);
                }
            }
        }

        for (i = 0; i < slots.size(); i++) 
            predictClass.insert(std::pair<MT_GESTURE_CLASS, int>(predictSlot(
                                slotPoints[i]), i));

#if DEBUG
        std::cout << "DEBUG: predict classification size " << 
            predictClass.size() << std::endl;
#endif
        if (predictClass.size() == 1) {
            predictIter = predictClass.begin();
            predictReturn = (*predictIter).first;
        } else if (predictClass.size() > 1) {
            predictIter = predictClass.begin();
            i = (*predictIter).second;
            predictIter++;
            j = (*predictIter).second;
#if DEBUG
            std::cout << "DEBUG: predict with slot " << i << " and " << j << 
                std::endl;
#endif
            point_t cp = getCentralPoint(slotPoints[i], slotPoints[j], isEllipse);
            if (isEllipse) 
                predictReturn = predictEllipse(slotPoints[i], slotPoints[j]);
            else 
                predictReturn = predictSlot(slotPoints[i], slotPoints[j]);
        }

        if (slotPoints) {
            delete [] slotPoints;
            slotPoints = NULL;
        }

        return predictReturn;
    }
}
