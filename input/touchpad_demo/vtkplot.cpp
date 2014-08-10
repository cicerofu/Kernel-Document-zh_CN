/*                                                                              
 * Copyright (C) 2013 Leslie Zhai <zhaixiang@linuxdeepin.com>
 */

#include <iostream>
#include <fstream>                                                              
#include <set>                                                                  
#include <vector>
#include <stdio.h>
#include <stdlib.h>
#include <vtkVersion.h>
#include <vtkSmartPointer.h>
#include <vtkAxis.h>
#include <vtkChartXY.h>
#include <vtkContextScene.h>
#include <vtkContextView.h>
#include <vtkFloatArray.h>
#include <vtkPlotPoints.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTable.h>

#define BUFFER_SIZE 1024

typedef struct {                                                                
    int x;                                                                      
    int y;                                                                      
} point_t;                                                                      
                                                                                
typedef struct {                                                                
    int slot;                                                                   
    point_t point;                                                              
} sample_t;

static std::set<int> slots;

static void m_multi_slots(std::set<int> slots, 
                          std::vector<sample_t> samples, 
                          int only_shown_slot);

int main(int argc, char* argv[])
{
    std::ifstream file;                                                         
    int slot;                                                                   
    int x, y;                                                                   
    int only_shown_slot = argv[2] ? atoi(argv[2]) : -1;
    std::vector<sample_t> samples;                                              
    std::vector<sample_t>::iterator sample_iter;                                

    if (argc < 2) {
        std::cout << "usage: <sample file> only_shown_slot" << std::endl;
        return -1;                                                              
    }

    file.open(argv[1]);                                                         
    while (file >> slot >> x >> y) {
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

    std::cout << "DEBUG: sample size " << samples.size()  << std::endl;         
    for (sample_iter = samples.begin(); 
         sample_iter < samples.end(); 
         sample_iter++) {
        slots.insert((*sample_iter).slot);                                      
    }                                                                           
    std::cout << "DEBUG: slot size " << slots.size() << std::endl;              
    if (slots.size() < 1)                                                       
        return -1;

    if (only_shown_slot != -1 && slots.size() - 1 < only_shown_slot) {
        std::cout << "only_shown_slot is out of range" << std::endl; 
        return -1;
    }

    m_multi_slots(slots, samples, only_shown_slot);

    return 0;
} 

static vtkSmartPointer<vtkTable> m_create_table(int slot, 
                                                std::vector<point_t> points) 
{
    vtkSmartPointer<vtkTable> table = vtkSmartPointer<vtkTable>::New();
    vtkSmartPointer<vtkFloatArray> arrX = vtkSmartPointer<vtkFloatArray>::New();
    vtkSmartPointer<vtkFloatArray> arrY = vtkSmartPointer<vtkFloatArray>::New();
    std::vector<point_t>::iterator iter;
    char buffer[BUFFER_SIZE];
    int i;

    arrX->SetName("X Axis");
    table->AddColumn(arrX);

    memset(buffer, 0, BUFFER_SIZE);
    snprintf(buffer, BUFFER_SIZE, "Slot_%d", slot);
    arrY->SetName(buffer);
    table->AddColumn(arrY);

    table->SetNumberOfRows(points.size());

    for (i = 0, iter = points.begin(); iter != points.end(); i++, iter++) {
        table->SetValue(i, 0, (*iter).x);
        table->SetValue(i, 1, (*iter).y);                                 
    }

    return table;
}

static void m_multi_slots(std::set<int> slots, 
                          std::vector<sample_t> samples, 
                          int only_shown_slot) 
{
    std::set<int>::iterator slot_iter;                                          
    std::vector<sample_t>::iterator sample_iter;
    std::vector<point_t>::iterator point_iter;    
    std::vector<point_t>* slot_points = new std::vector<point_t>[slots.size()];
    vtkPlot* points = NULL;
    int i;
    
    // Set up a 2D scene, add an XY chart to it
    vtkSmartPointer<vtkContextView> view = vtkSmartPointer<vtkContextView>::New();
    view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
    view->GetRenderWindow()->SetSize(800, 600);

    vtkSmartPointer<vtkChartXY> chart = vtkSmartPointer<vtkChartXY>::New();
    view->GetScene()->AddItem(chart);
    chart->SetShowLegend(true);
    chart->SetDrawAxesAtOrigin(false);

    for (slot_iter = slots.begin(); slot_iter != slots.end(); slot_iter++) {
        for (sample_iter = samples.begin(); 
             sample_iter != samples.end(); 
             sample_iter++) {
            if (*slot_iter == (*sample_iter).slot) {
                point_t point;                                                  
                point.x = (*sample_iter).point.x;                               
                point.y = (*sample_iter).point.y;                               
                slot_points[*slot_iter].push_back(point);                       
            }                                                                   
        }
        printf("DEBUG: slot_points[%d] size %d\n", 
               *slot_iter, slot_points[*slot_iter].size());
    }

    // Centrol points
    if (slots.size() == 2) {
        point_t slot1_point = slot_points[0][slot_points[0].size() - 1];
        point_t slot2_point = slot_points[1][slot_points[1].size() - 1];
        printf("DEBUG: central point (x, y) = (%d, %d)\n", 
               abs(slot1_point.x - slot2_point.x) / 2 + std::min(slot1_point.x, slot2_point.x), 
               abs(slot1_point.y - slot2_point.y) / 2 + std::min(slot1_point.y, slot2_point.y));
    }

    for (slot_iter = slots.begin(); slot_iter != slots.end(); slot_iter++) {
        if (only_shown_slot != -1) {
            if (only_shown_slot != *slot_iter) 
                continue;
        }
        points = chart->AddPlot(vtkChart::POINTS);
        points->SetInput(m_create_table(*slot_iter, slot_points[*slot_iter]), 0, 1);
        switch (*slot_iter) {
        case 4:
            points->SetColor(255, 255, 0, 255);
            break;
        case 3:
            points->SetColor(255, 0, 255, 255);
            break;
        case 2:
            points->SetColor(0, 255, 0, 255);
            break;
        case 1:
            points->SetColor(255, 0, 0, 255);
            break;
        case 0:
        default:
            points->SetColor(0, 0, 255, 255);
        }
        points->SetWidth(1.0);                                              
        vtkPlotPoints::SafeDownCast(points)->SetMarkerStyle(vtkPlotPoints::CIRCLE);
    }
    //Finally render the scene
    view->GetRenderWindow()->SetMultiSamples(0);
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
  
    delete [] slot_points;                                                      
    slot_points = NULL;
}
