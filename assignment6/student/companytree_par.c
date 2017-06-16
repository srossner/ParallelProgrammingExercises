#include "companytree.h"
#include <omp.h>
#include "stdio.h"

void traverse(tree *node, int numThreads){


    if(node != NULL){
        #pragma omp parallel sections shared(node, numThreads)
        {

#pragma omp section
            traverse(node->right, numThreads);

#pragma omp section
            traverse(node->left, numThreads);



#pragma omp section
        node->work_hours = compute_workHours(node->data);
        }
        top_work_hours[node->id] = node->work_hours;




    }


}

