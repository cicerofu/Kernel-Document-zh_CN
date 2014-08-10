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

#include <vector>
#include <set>

namespace MTGR 
{
	typedef struct 
    {
		int x;  /* ABS_MT_POSITION_X */
		int y;  /* ABS_MT_POSITION_Y */
	} point_t;

	typedef struct 
    {
		int slot;       /* ABS_MT_SLOT means which finger to touch */
		point_t point;
	} sample_t;

    typedef enum 
    {
        NULL_GESTURE = 0, 
        RIGHTWARD, 
        LEFTWARD, 
        UPWARD, 
        DOWNWARD, 
        ZOOMIN, 
        ZOOMOUT, 
        CLOCKWISE,          /* do not supporte clockwise, anti-clockwise yet */
        ANTICLOCKWISE, 
        FINGER_LONG_PRESS, 
    } MT_GESTURE_CLASS;

    class GestureRecognition 
    {
    public:
        GestureRecognition();
        virtual ~GestureRecognition();

    public:
        MT_GESTURE_CLASS predict(std::vector<sample_t> samples);

    private:
        point_t getCentralPoint(std::vector<point_t> points1, 
                                std::vector<point_t> points2, 
                                bool & isEllipse);
        MT_GESTURE_CLASS predictSlot(std::vector<point_t> slot_points);
        MT_GESTURE_CLASS predictSlot(std::vector<point_t> points1, 
                                     std::vector<point_t> points2);
        MT_GESTURE_CLASS predictEllipse(std::vector<point_t> points1, 
                                        std::vector<point_t> points2);

    private:
        std::set<int> slots;
    };
};
