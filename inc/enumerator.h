/*!
 * @file /enumerator.h
 * @brief simple enumerator of arrays
 * @details
 * The purpose of this is to allow multiple independent clients to enumerate the 
 * elements of an array simultaneously by having their own counter that they use.
 *
 * DEFINE_ENUMERATOR goes into a header file and IMPLEMENT_ENUMERATOR into the C file.
 * Both define a name_enumerator_t type (which is just a counter) as well as 
 * name_enumerator_first and name_enumerator_next functions.
 *
 # Example usage:
 *
 *   #define MAX_INDEX 100;
 *   int array[MAX_INDEX];
 *   DEFINE_ENUMERATOR(int, array_enumerator)
 *   IMPLEMENT_ENUMERATOR(int, array_enumerator, array, MAX_INDEX)
 * 
 *   array_enumerator_t e;
 *   int * value;
 *   for (value = array_enumerator_first(&e); value != null; value = array_enumerator_next(&e)) 
 *       printf("value is %d\n", *value);
 *
 * The last 4 statements could also be done using the FOR_ENUMERATION helper macro as:
 *
 *   FOR_ENUMERATION(value, int, array_enumerator) printf("value is %d\n", *value);
 * 
 *  fine-print: copyright 2023 David Hamilton. This is free software (see LICENSE.txt in root directory), provided "AS IS" without any warranty, express or implied.
 */

#ifndef ENUMERATOR_H
#define ENUMERATOR_H

#define DEFINE_ENUMERATOR(T, NAME)   typedef int NAME ## _enumerator_t;                            \
                                     T * NAME ## _first(NAME ## _enumerator_t * e);                \
                                     T * NAME ## _next(NAME ## _enumerator_t * e);

#define IMPLEMENT_ENUMERATOR(T, NAME, A, MAX)   T * NAME ## _first(NAME ## _enumerator_t * e) {    \
                                                   (*e) = 1;                                       \
                                                   return &(A[0]);                                 \
                                                }                                                  \
                                                T * NAME ## _next(NAME ## _enumerator_t * e) {     \
                                                    T* temp;                                       \
                                                    if (*e < MAX) temp = &(A[*e]);                 \
                                                    else return NULL;                              \
                                                    *e = *e + 1;                                   \
                                                    if (*e > MAX) *e = MAX;                        \
                                                    return temp;                                   \
                                                }
#define FOR_ENUMERATION(PTR, T, NAME)                                                                 \
T * PTR;                                                                                           \
NAME ## _enumerator_t PTR ## e;                                                                        \
for (PTR = NAME ## _first(&PTR ## e); PTR != NULL; PTR = NAME ## _next(&PTR ## e))          

#endif