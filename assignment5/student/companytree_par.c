#include "companytree.h"


void traverse(tree *node, int numThreads){

{

        omp_set_max_active_levels( numThreads );
    if(node != NULL){

        tree *right = node->right;
        tree *left = node->left;


        #pragma omp task shared(right, numThreads)
        traverse(right, numThreads);

        #pragma omp task shared(left, numThreads)
        traverse(left, numThreads);

        node->work_hours = compute_workHours(node->data);
        top_work_hours[node->id] = node->work_hours;
    }
}


}

