#pragma once
#include <inttypes.h>

// Running median value filter.
//
// Insert values as they arrive.  Ask for the median when needed.
// Sorting is done on insert O(n) and median return is very fast.
//
// Code originally from FastMedianFilter (public domain) post on the
// arduino forum.  http://forum.arduino.cc/index.php?topic=53081.0
//
// Adapted to support less than NUM values (i.e. no default required),
// and clear method.
//
//= EXAMPLE
//
//   MedianFilter< uint8_t, 10 > filter;
//   filter.add( 5 );
//   filter.add( 3 );
//   filter.add( 7 );
//   assert( filter.median() == 5 );
//
template < typename ValueType, uint8_t NUM >
class MedianFilter
{
public:
   MedianFilter();;

   void add( ValueType value );
   void clear();
   
   ValueType median();
   
private:
   // Index in m_values of the last added element.
   uint8_t m_inputIdx;
   
   // Index of the median value in m_sorted.
   uint8_t m_medianIdx;

   // Number of values that have been input.  Always <= NUM.
   uint8_t m_num;

   // Circular buffer of input values.
   ValueType m_values[NUM];
   
   // Sorted version of m_values.
   ValueType m_sorted[NUM];
};

//============================================================================
// Constructor
//
template < typename ValueType, uint8_t NUM >
inline
MedianFilter< ValueType, NUM >::
MedianFilter()
   : m_inputIdx( 0 ),
     m_medianIdx( 0 ),
     m_num( 0 )
{
   // initial, fake median value in case median() is called before
   // add().
   m_sorted[0] = 0; 
}

//============================================================================
// Return the median value of the last NUM inserved values.
//
// If no values have been inserted, 0 is returend.
//
template < typename ValueType, uint8_t NUM >
inline
ValueType
MedianFilter< ValueType, NUM >::
median()
{
   return m_sorted[m_medianIdx];
}

//============================================================================
// Clear all the values.
template < typename ValueType, uint8_t NUM >
inline
void
MedianFilter< ValueType, NUM >::
clear()
{
   m_inputIdx = 0;
   m_medianIdx = 0;
   m_num = 0;
   m_sorted[0] = 0;
}

//============================================================================
// Add a value to the filter.
//
template < typename ValueType, uint8_t NUM >
inline
void
MedianFilter< ValueType, NUM >::
add( ValueType value )
{
   bool sortUp = false;
   uint8_t sortIdx;
   
   // If we haven't filled the buffer yet, increment the counters.
   if ( m_num < NUM )
   {
      m_inputIdx = m_num++;
      m_medianIdx = m_num / 2;

      // fill the new value in the cyclic buffer
      m_values[m_inputIdx] = value;
      
      // leave sortUp = false - we're appending to the end and need to
      // do a sort down the array.
      sortIdx = m_inputIdx;
   }
   // Otherwise, we need to use m_inputs as a circular buffer and
   // replace the old value with the new one.
   else 
   {
      if ( ++m_inputIdx == NUM )
      {
         m_inputIdx = 0;
      }

      // Get the current value to replace.
      ValueType oldValue = m_values[m_inputIdx];

      // If the value is unchanged, do nothing.
      if ( value == oldValue )
      {
         return;
      }

      // Save the new value.
      m_values[m_inputIdx] = value;

      // Find the old value in the sorted array so we know where to
      // trigger the starting sort from.
      for ( sortIdx = 0; sortIdx < NUM; sortIdx++ )
      {
         if ( m_sorted[sortIdx] == oldValue )
         {
            break;
         }
      }

      // Figure out which direction we need to sort.
      if ( value > oldValue )
      {
         sortUp = true;
      }
   }

   // sortIdx is the index of the oldValue in the sorted buffer -
   // replace the value
   m_sorted[sortIdx] = value;
   
   // New value is bigger than the old one, bubble sort upwardsa
   if ( sortUp )
   {
      for( uint8_t p=sortIdx, q=sortIdx+1; q < m_num; p++, q++ )
      {
         if ( m_sorted[p] > m_sorted[q] )
         {
            ValueType tmp = m_sorted[p];
            m_sorted[p] = m_sorted[q];
            m_sorted[q] = tmp;
         }
         else
         {
            return;
         }
      }
   }
   // New value is smaller than the old one, bubble sort downwards.
   else
   {
      for( int p=sortIdx-1, q=sortIdx; q > 0; p--, q-- )
      {
         if ( m_sorted[p] > m_sorted[q] )
         {
            ValueType tmp = m_sorted[p];
            m_sorted[p] = m_sorted[q];
            m_sorted[q] = tmp;
         }
         else
         {
            return;
         }
      }
   }
}

//============================================================================
