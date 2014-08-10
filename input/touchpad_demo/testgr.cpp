/*                                                                              
 * Copyright (C) 2013 Leslie Zhai <zhaixiang@linuxdeepin.com>                    
 */

#include <iostream>
#include <fstream>                                                              

#include "gesture_recognition.h"

using namespace MTGR;

int main(int argc, char* argv[])
{
    std::ifstream file;                                                         
    int slot;                                                                   
    int x, y;                                                                   
    std::vector<sample_t> samples;
    GestureRecognition gr;

    if (argc < 2) {
        std::cout << "usage: <sample file>" << std::endl;                       
        return -1;                                                              
    }

    file.open(argv[1]);                                                         
    while (file >> slot >> x >> y)                                              
    {                                                                           
        if (x < 0 || y < 0)                                                     
            continue;                                                           
        point_t point;                                                          
        point.x = x;                                                            
        point.y = y;                                                            
        sample_t sample;                                                        
        sample.slot = slot;                                                     
        sample.point = point;                                                   
        samples.push_back(sample);                                              
    }                                                                           
    file.close();

    switch (gr.predict(samples)) {
    case 0:
        std::cout << "DEBUG: predict NULL" << std::endl;
        break;
    case 1:
        std::cout << "DEBUG: predict rightward" << std::endl;
        break;
    case 2:
        std::cout << "DEBUG: predict leftward" << std::endl;
        break;
    case 3:
        std::cout << "DEBUG: predict upward" << std::endl;
        break;
    case 4:
        std::cout << "DEBUG: predict downward" << std::endl;
        break;
    case 5:
        std::cout << "DEBUG: predict zoom in" << std::endl;
        break;
    case 6:
        std::cout << "DEBUG: predict zoom out" << std::endl;
        break;
    case 7:
        std::cout << "DEBUG: predict rotate clockwise" << std::endl;
        break;
    case 8:
        std::cout << "DEBUG: predict rotate anti-clockwise" << std::endl;
        break;
    }

    return 0;
} 
