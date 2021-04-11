/**
  * @file EnigmaIOTRingBuffer.h
  * @version 0.9.7
  * @date 04/02/2021
  * @author German Martin
  * @brief Library to build a gateway for EnigmaIoT system
  */

#ifndef _ENIGMAIOTBUFFER_h
#define _ENIGMAIOTBUFFER_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "helperFunctions.h"

/**
  * @brief Ring buffer class. Used to implement message buffer
  *
  */
template <typename Telement>
class EnigmaIOTRingBuffer {
protected:
    int maxSize; ///< @brief Buffer size
    int numElements = 0; ///< @brief Number of elements that buffer currently has
    int readIndex = 0; ///< @brief Pointer to next item to be read
    int writeIndex = 0; ///< @brief Pointer to next position to write onto
    Telement* buffer; ///< @brief Actual buffer

public:
    /**
      * @brief Creates a ring buffer to hold `Telement` objects
      * @param range Buffer depth
      */
    EnigmaIOTRingBuffer <Telement> (int range) : maxSize (range) {
        buffer = new Telement[maxSize];
    }

    /**
      * @brief EnigmaIOTRingBuffer destructor
      * @param range Free up buffer memory
      */
    ~EnigmaIOTRingBuffer () {
        maxSize = 0;
        delete[] (buffer);
    }

    /**
      * @brief Returns actual number of elements that buffer holds
      * @return Returns Actual number of elements that buffer holds
      */
    int size () { return numElements; }

    /**
      * @brief Checks if buffer is full
      * @return Returns `true`if buffer is full, `false` otherwise
      */
    bool isFull () { return numElements == maxSize; }

    /**
      * @brief Checks if buffer is empty
      * @return Returns `true`if buffer has no elements stored, `false` otherwise
      */
    bool empty () { return (numElements == 0); }

    /**
      * @brief Adds a new item to buffer, deleting older element if it is full
      * @param item Element to add to buffer
      * @return Returns `false` if buffer was full before inserting the new element, `true` otherwise
      */
    bool push (Telement* item) {
        bool wasFull = isFull ();
        DEBUG_DBG ("Add element. Buffer was %s", wasFull ? "full" : "not full");
        DEBUG_DBG ("Before -- > ReadIdx: %d. WriteIdx: %d. Size: %d", readIndex, writeIndex, numElements);
#ifdef ESP32
        portMUX_TYPE myMutex = portMUX_INITIALIZER_UNLOCKED;
        portENTER_CRITICAL (&myMutex);
#endif
        memcpy (&(buffer[writeIndex]), item, sizeof (Telement));
        //Serial.printf ("Copied: %d bytes\n", sizeof (Telement));
        writeIndex++;
        if (writeIndex >= maxSize) {
            writeIndex %= maxSize;
        }
        if (wasFull) { // old value is no longer valid
            readIndex++;
            if (readIndex >= maxSize) {
                readIndex %= maxSize;
            }
        } else {
            numElements++;
        }
#ifdef ESP32
        portEXIT_CRITICAL (&myMutex);
#endif
        DEBUG_DBG ("After -- > ReadIdx: %d. WriteIdx: %d. Size: %d", readIndex, writeIndex, numElements);
        return !wasFull;
    }

    /**
      * @brief Deletes older item from buffer, if buffer is not empty
      * @return Returns `false` if buffer was empty before trying to delete element, `true` otherwise
      */
    bool pop () {
        bool wasEmpty = empty ();
        DEBUG_DBG ("Remove element. Buffer was %s", wasEmpty ? "empty" : "not empty");
        DEBUG_DBG ("Before -- > ReadIdx: %d. WriteIdx: %d. Size: %d", readIndex, writeIndex, numElements);
        if (!wasEmpty) {
            readIndex++;
            if (readIndex >= maxSize) {
                readIndex %= maxSize;
            }
            numElements--;
        }
        DEBUG_DBG ("After -- > ReadIdx: %d. WriteIdx: %d. Size: %d", readIndex, writeIndex, numElements);
        return !wasEmpty;
    }

    /**
      * @brief Gets a pointer to older item in buffer, if buffer is not empty
      * @return Returns pointer to element. If buffer was empty before calling this method it returns `NULL`
      */
    Telement* front () {
        DEBUG_DBG ("Read element. ReadIdx: %d. WriteIdx: %d. Size: %d", readIndex, writeIndex, numElements);
        if (!empty ()) {
            return &(buffer[readIndex]);
        } else {
            return NULL;
        }
    }
};

#endif