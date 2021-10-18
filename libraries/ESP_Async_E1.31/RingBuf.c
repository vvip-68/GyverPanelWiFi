/*
  RingBuf.c - Library for implementing a simple Ring Buffer on Arduino boards.
  Created by D. Aaron Wisner (daw268@cornell.edu)
  January 17, 2015.
  Released into the public domain.
*/
#include "RingBuf.h"
#include <string.h>

/////// Constructor //////////
RingBuf *RingBuf_new(int size, int len)
{
  RingBuf *self = (RingBuf *)malloc(sizeof(RingBuf));
  if (!self) return NULL;
  memset(self, 0, sizeof(RingBuf));
  if (RingBuf_init(self, size, len) < 0)
  {
    free(self);
    return NULL;
  }
  return self;
}

int RingBuf_init(RingBuf *self, int size, int len)
{
  self->buf = (unsigned char *)malloc(size*len);
  if (!self->buf) return -1;
  memset(self->buf, 0, size*len);

  self->size = size;
  self->len = len;
  self->start = 0;
  self->end = 0;
  self->elements = 0;

  self->next_end_index = &RingBufNextEndIndex;
  self->incr_end_index = &RingBufIncrEnd;
  self->incr_start_index = &RingBufIncrStart;
  self->isFull = &RingBufIsFull;
  self->isEmpty = &RingBufIsEmpty;
  self->add = &RingBufAdd;
  self->numElements = &RingBufNumElements;
  self->peek = &RingBufPeek;
  self->pull = &RingBufPull;
  return 0;
}
/////// Deconstructor //////////
int RingBuf_delete(RingBuf *self)
{
    free(self->buf);
    free(self);
  return 0;
}

/////// PRIVATE METHODS //////////

// get next empty index
int RingBufNextEndIndex(RingBuf *self)
{
  //buffer is full
  if (self->isFull(self)) return -1;
  //if empty dont incriment
  return (self->end+(unsigned int)!self->isEmpty(self))%self->len;
}

// incriment index of RingBuf struct, only call if safe to do so
int RingBufIncrEnd(RingBuf *self)
{
  self->end = (self->end+1)%self->len;
  return self->end;
}


// incriment index of RingBuf struct, only call if safe to do so
int RingBufIncrStart(RingBuf *self)
{
  self->start = (self->start+1)%self->len;
  return self->start;
}

/////// PUBLIC METHODS //////////

// Add an object struct to RingBuf
int RingBufAdd(RingBuf *self, const void *object)
{
  int index;
  // Perform all atomic opertaions
   RB_ATOMIC_START
  {
    index = self->next_end_index(self);
    //if not full
    if (index >= 0)
    {
       memcpy(self->buf + index*self->size, object, self->size);
      if (!self->isEmpty(self)) self->incr_end_index(self);
      self->elements++;
    }
  }
  RB_ATOMIC_END

  return index;
}

// Return pointer to num element, return null on empty or num out of bounds
void *RingBufPeek(RingBuf *self, unsigned int num)
{
  void *ret = NULL;
  // Perform all atomic opertaions
  RB_ATOMIC_START
  {
    //empty or out of bounds
    if (self->isEmpty(self) || num > self->elements - 1) ret = NULL;
    else ret = &self->buf[((self->start + num)%self->len)*self->size];
  }
  RB_ATOMIC_END

  return ret;
}

// Returns and removes first buffer element
void *RingBufPull(RingBuf *self, void *object)
{
  void *ret = NULL;
  // Perform all atomic opertaions
  RB_ATOMIC_START
  {
    if (self->isEmpty(self)) ret = NULL;
    // Else copy Object
    else
    {
      memcpy(object, self->buf+self->start*self->size, self->size);
      self->elements--;
      // don't incriment start if removing last element
      if (!self->isEmpty(self)) self->incr_start_index(self);
      ret = object;
    }
  }
  RB_ATOMIC_END

  return ret;
}

// Returns number of elemnts in buffer
unsigned int RingBufNumElements(RingBuf *self)
{
  unsigned int elements;

  // Perform all atomic opertaions
  RB_ATOMIC_START
  {
    elements = self->elements;
  }
  RB_ATOMIC_END

  return elements;
}

// Returns true if buffer is full
bool RingBufIsFull(RingBuf *self)
{
  bool ret;

  // Perform all atomic opertaions
  RB_ATOMIC_START
  {
    ret = self->elements == self->len;
  }
  RB_ATOMIC_END

  return ret;
}

// Returns true if buffer is empty
bool RingBufIsEmpty(RingBuf *self)
{
  bool ret;

  // Perform all atomic opertaions
  RB_ATOMIC_START
  {
    ret = !self->elements;
  }
  RB_ATOMIC_END

  return ret;
}
