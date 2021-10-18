/*
  RingBuf.h - Library for implementing a simple Ring Buffer on Arduino boards.
  Created by D. Aaron Wisner (daw268@cornell.edu)
  January 17, 2015.
  Released into the public domain.
*/
#ifndef RingBuf_h
#define RingBuf_h

#ifdef ARDUINO
    #include <Arduino.h>
#else
    #include <stdint.h>
#endif

#ifndef __cplusplus
#ifndef bool
    #define bool uint8_t
#endif
#endif


#ifdef ARDUINO

    #if defined(ARDUINO_ARCH_AVR)
        #include <util/atomic.h>
        #define RB_ATOMIC_START ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
        #define RB_ATOMIC_END }


    #elif defined(ARDUINO_ARCH_ESP8266)
        #ifndef __STRINGIFY
            #define __STRINGIFY(a) #a
        #endif

        #ifndef xt_rsil
            #define xt_rsil(level) (__extension__({uint32_t state; __asm__ __volatile__("rsil %0," __STRINGIFY(level) : "=a" (state)); state;}))
        #endif

        #ifndef xt_wsr_ps
            #define xt_wsr_ps(state)  __asm__ __volatile__("wsr %0,ps; isync" :: "a" (state) : "memory")
        #endif

        #define RB_ATOMIC_START do { uint32_t _savedIS = xt_rsil(15) ;
        #define RB_ATOMIC_END xt_wsr_ps(_savedIS); } while(0);

    #else
        #define RB_ATOMIC_START {
        #define RB_ATOMIC_END }
        #warning "This library only fully supports AVR and ESP8266 Boards."
        #warning "Operations on the buffer in ISRs are not safe!"
    #endif

#else
    #define RB_ATOMIC_START {
    #define RB_ATOMIC_END }
    #warning "Operations on the buffer in ISRs are not safe!"
    #warning "Impliment RB_ATOMIC_START and RB_ATOMIC_END macros for safe ISR operation!"
#endif



typedef struct RingBuf RingBuf;

typedef struct RingBuf
{
  // Invariant: end and start is always in bounds
  unsigned char *buf;
  unsigned int len, size, start, end, elements;

  // Private:
  int (*next_end_index) (RingBuf*);
  int (*incr_end_index) (RingBuf*);

  int (*incr_start_index) (RingBuf*);

  //public:
  // Returns true if full
  bool (*isFull) (RingBuf*);
  // Returns true if empty
  bool (*isEmpty) (RingBuf*);
  // Returns number of elemnts in buffer
  unsigned int (*numElements)(RingBuf*);
  // Add Event, Returns index where added in buffer, -1 on full buffer
  int (*add) (RingBuf*, const void*);
  // Returns pointer to nth element, NULL when nth element is empty
  void *(*peek) (RingBuf*, unsigned int);
  // Removes element and copies it to location pointed to by void *
  // Returns pointer passed in, NULL on empty buffer
  void *(*pull) (RingBuf*, void *);

} RingBuf;

#ifdef __cplusplus
extern "C" {
#endif

RingBuf *RingBuf_new(int size, int len);
int RingBuf_init(RingBuf *self, int size, int len);
int RingBuf_delete(RingBuf *self);

int RingBufNextEndIndex(RingBuf *self);
int RingBufIncrEnd(RingBuf *self);
int RingBufIncrStart(RingBuf *self);
int RingBufAdd(RingBuf *self, const void *object);
void *RingBufPeek(RingBuf *self, unsigned int num);
void *RingBufPull(RingBuf *self, void *object);
bool RingBufIsFull(RingBuf *self);
bool RingBufIsEmpty(RingBuf *self);
unsigned int RingBufNumElements(RingBuf *self);

#ifdef __cplusplus
}
#endif


// For those of you who cant live without pretty C++ objects....
#ifdef __cplusplus
class RingBufC
{

public:
    RingBufC(int size, int len) { buf = RingBuf_new(size, len); }
    ~RingBufC() { RingBuf_delete(buf); }

    bool isFull() { return RingBufIsFull(buf); }
    bool isEmpty() { return RingBufIsEmpty(buf); }
    unsigned int numElements() { return RingBufNumElements(buf); }

    unsigned int add(const void *object) { return RingBufAdd(buf, object); }
    void *peek(unsigned int num) { return RingBufPeek(buf, num); }
    void *pull(void *object) { return RingBufPull(buf, object); }

    // Use this to check if memory allocation failed
    bool allocFailed() { return !buf; }

private:
    RingBuf *buf;
};
#endif


#endif
