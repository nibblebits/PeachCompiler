#ifndef VECTOR_H
#define VECTOR_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>

// We want at least 20 vector element spaces in reserve before having
// to reallocate memory again
#define VECTOR_ELEMENT_INCREMENT 20

enum
{
    VECTOR_FLAG_PEEK_DECREMENT = 0b00000001
};

struct vector
{
    void* data;
    // The pointer index is the index that will be read next upon calling "vector_peek".
    // This index will then be incremented
    int pindex;
    int rindex;
    int mindex;
    int count;
    int flags;
    size_t esize;


    // Vector of struct vector, holds saves of this vector. YOu can save the internal state
    // at all times with vector_save
    // Data is not restored and is permenant, save does not respect data, only pointers
    // and variables are saved. Useful to temporarily push the vector state
    // and restore it later.
    struct vector* saves;
};


struct vector* vector_create(size_t esize);
void vector_free(struct vector* vector);
void* vector_at(struct vector* vector, int index);
void* vector_peek_ptr_at(struct vector* vector, int index);
void* vector_peek_no_increment(struct vector* vector);
void* vector_peek(struct vector* vector);
void *vector_peek_at(struct vector *vector, int index);
void vector_set_flag(struct vector* vector, int flag);
void vector_unset_flag(struct vector* vector, int flag);

/**
 * Pops off the last peeked element
 */
void vector_pop_last_peek(struct vector* vector);

/**
 * Peeks into the vector of pointers, returning the pointer value its self
 * 
 * Use this function instead of vector_peek if this is a vector of pointers
 */
void* vector_peek_ptr(struct vector* vector);
void vector_set_peek_pointer(struct vector* vector, int index);
void vector_set_peek_pointer_end(struct vector* vector);
void vector_push(struct vector* vector, void* elem);
void vector_push_at(struct vector *vector, int index, void *ptr);
void vector_pop(struct vector* vector);
void vector_peek_pop(struct vector* vector);

void* vector_back(struct vector* vector);
void *vector_back_or_null(struct vector *vector);

void* vector_back_ptr(struct vector* vector);
void* vector_back_ptr_or_null(struct vector* vector);

/**
 * Returns the vector data as a char pointer 
 */
const char* vector_string(struct vector* vec);

/**
 * Returns true if this vector is empty
 */
bool vector_empty(struct vector* vector);
void vector_clear(struct vector* vector);

int vector_count(struct vector* vector);
/**
 * freads from the file directly into the vector
 */
int vector_fread(struct vector* vector, int amount, FILE* fp);
/**
 * Returns a void pointer pointing to the data of this vector
 */
void* vector_data_ptr(struct vector* vector);

int vector_insert(struct vector *vector_dst, struct vector *vector_src, int dst_index);

/**
 * Pops the element at the given data address.
 * \param vector The vector to pop an element on
 * \param address The address that is part of the vector->data range to pop off.
 * \return Returns the index that we popped off.
 */
int vector_pop_at_data_address(struct vector* vector, void* address);

/**
 * Pops the given value from the vector. Only the first value found is popped
 */
int vector_pop_value(struct vector* vector, void* val);

void vector_pop_at(struct vector *vector, int index);

/**
 * Decrements the peek pointer so that the next peek
 * will point at the last peeked token
 */
void vector_peek_back(struct vector* vector);

/**
 * Returns the current index that a vector_push would push too
 */
int vector_current_index(struct vector* vector);

/**
 * Saves the state of the vector
 */
void vector_save(struct vector* vector);
/**
 * Restores the state of the vector
 */
void vector_restore(struct vector* vector);

/**
 * Purges the last save state as if it never happend
 */
void vector_save_purge(struct vector* vector);



/**
 * Returns the element size per element in this vector
 */
size_t vector_element_size(struct vector* vector);


/**
 * Clones the given vector including all vector data, saves are ignored
 */
struct vector* vector_clone(struct vector* vector);

#endif