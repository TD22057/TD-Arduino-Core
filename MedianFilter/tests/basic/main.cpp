#include "../../MedianFilter/MedianFilter.h"
#include <iostream>

// Compile and run:
// g++ -o test main.cpp
// ./test

int
main()
{
   MedianFilter< int, 5 > m5;

   const int NUM = 10;
   const int value[NUM] = {
      10, 2, 4, 1, 2, 3, 6, 9, 8, 2
   };
   // NUM RIGHT  BUFFER  
   //  0   10    10  
   //  1   10    10  2  
   //  2    4    10  2  4  
   //  3    4    10  2  4  1  
   //  4    2    10  2  4  1  2 
   //  5    2     2  4  1  2  3 
   //  6    3     4  1  2  3  6 
   //  7    3     1  2  3  6  9 
   //  8    6     2  3  6  9  8 
   //  9    6     3  6  9  8  2
   const int right[NUM] = {
      10, 10, 4, 4, 2, 2, 3, 3, 6, 6
   };

   bool fail = false;

   for ( int i = 0; i < 2; i++ )
   {
      for ( int j = 0; j < NUM; j++ )
      {
         m5.add( value[j] );
         int v = m5.median();
         if ( v != right[j] )
         {
            fail = true;
            std::cout << "Error at i=" << i << " j=" << j << " median=" << v
                      << " right=" << right[j] << "\n";
         }
      }

      if ( fail )
      {
         break;
      }
      m5.clear();
   }

   if ( ! fail )
   {
      std::cout << "Passed\n";
   }

   return 0;
}
