#include "companytree.h"
#include <math.h>

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

void traverse(tree *node, int numThreads){

    if(node != NULL){

        tree *right = node->right;
        tree *left = node->left;

        #pragma omp task shared(right, numThreads)
        traverse(right, numThreads);

        #pragma omp task shared(left, numThreads)
        traverse(left, numThreads);

        top_work_hours[node->id] = compute_workHours_my(node->data);

    }
}

