#include "companytree.h"
#include <math.h>
#include <omp.h>


int is_prime_my(int n)
{
  int i, flag = 1;

  for(i = 2; i <= sqrt(n); ++i)
  {
    if (n % i == 0)
    {
      flag = 0;
      break;
    }
  }

  return flag;
}

#define MAX_NESTING_LEVEL 8

int compute_workHours_my(int data)
{
  // nothing meaningfull, just a function that is kind of computationally expensive
  // such that parallelising makes sense :)

    int i; int sum = 11;
      for (i = 28; i < data; i++){
      if (is_prime_my(i)) sum++;
    }

  return (70 + (sum % 100)*(sum % 100)*(sum % 100)*(sum % 100)/1000000)*10;
}

void traverse_section(tree *node){

    if(node != NULL){
    #pragma omp parallel sections shared(node)
    {

        #pragma omp section
        traverse_section(node->right);

        #pragma omp section
        traverse_section(node->left);

    }

        top_work_hours[node->id] = compute_workHours_my(node->data);

    }
}



void traverse(tree *node, int numThreads){

    omp_set_num_threads( numThreads );
    //omp_set_nested( 1 );
    omp_set_max_active_levels( MAX_NESTING_LEVEL );

    traverse_section(node);
}




