/**
  * @file Comms_hal.h
  * @version 0.9.8
  * @date 15/07/2021
  * @author German Martin
  * @brief Generic communication system abstraction layer
  *
  * This is the interface that communication definition should implement to be used as layer 1 on EnigmaIoT
  */
#ifndef _COMMS_HAL_h
#define _COMMS_HAL_h

#if defined(ARDUINO) && ARDUINO >= 100
#include "Arduino.h"
#else
#include "WProgram.h"
#endif
#include "EnigmaIoTconfig.h"

  /**
	* @brief Peer role on communication
	*/
enum peerType_t {
	COMM_NODE = 0, /**< Peer acts as a node */
	COMM_GATEWAY = 1 /**< Peer acts as a gateway */
};

typedef struct {
    uint8_t dstAddress[ENIGMAIOT_ADDR_LEN]; /**< Message topic*/
    uint8_t payload[MAX_MESSAGE_LENGTH]; /**< Message payload*/
    size_t payload_len; /**< Payload length*/
} comms_queue_item_t;


typedef void (*comms_hal_rcvd_data)(uint8_t* address, uint8_t* data, uint8_t len, signed int rssi);
typedef void (*comms_hal_sent_data)(uint8_t* address, uint8_t status);

/**
  * @brief Interface for communication subsystem abstraction layer definition
  */
class Comms_halClass {
public:
	static const size_t COMMS_HAL_MAX_MESSAGE_LENGTH = 0; ///< @brief Maximum message length
	static const uint8_t COMMS_HAL_ADDR_LEN = 1; ///< @brief Address length

protected:
	uint8_t gateway[COMMS_HAL_ADDR_LEN]; ///< @brief Gateway address
	uint8_t channel; ///< @brief Comms channel to be used

	comms_hal_rcvd_data dataRcvd = 0; ///< @brief Pointer to a function to be called on every received message
	comms_hal_sent_data sentResult = 0; ///< @brief Pointer to a function to be called to notify last sending status
	peerType_t _ownPeerType; ///< @brief Stores peer type, node or gateway

	/**
	  * @brief Communication subsistem initialization
	  * @param peerType Role that peer plays into the system, node or gateway.
	  */
	virtual void initComms (peerType_t peerType) = 0;


public:
    Comms_halClass () {}
    
	/**
	  * @brief Setup communication environment and establish the connection from node to gateway
	  * @param gateway Address of gateway. It may be `NULL` in case this is used in the own gateway
	  * @param channel Establishes a channel for the communication. Its use depends on actual communications subsystem
	  * @param peerType Role that peer plays into the system, node or gateway.
	  */
	virtual void begin (uint8_t* gateway, uint8_t channel, peerType_t peerType = COMM_NODE) = 0;

	/**
	  * @brief Terminates communication and closes all connectrions
	  */
	virtual void stop () = 0;

	/**
	  * @brief Sends data to the other peer
	  * @param da Destination address to send the message to
	  * @param data Data buffer that contain the message to be sent
	  * @param len Data length in number of bytes
	  * @return Returns sending status. 0 for success, any other value to indicate an error.
	  */
	virtual int32_t send (uint8_t* da, uint8_t* data, int len) = 0;

	/**
	  * @brief Attach a callback function to be run on every received message
	  * @param dataRcvd Pointer to the callback function
	  */
	virtual void onDataRcvd (comms_hal_rcvd_data dataRcvd) = 0;

	/**
	  * @brief Attach a callback function to be run after sending a message to receive its status
	  * @param dataRcvd Pointer to the callback function
	  */
	virtual void onDataSent (comms_hal_sent_data dataRcvd) = 0;

	/**
	  * @brief Get address length that a specific communication subsystem uses
	  * @return Returns number of bytes that is used to represent an address
	  */
	virtual uint8_t getAddressLength () = 0;

    /**
      * @brief Sends next message in the queue
      */
    virtual void handle () = 0;

    /**
      * @brief Enables or disables transmission of queued messages. Used to disable communication during wifi scan
      * @param enable `true` to enable transmission, `false` to disable it
      */
    virtual void enableTransmit (bool enable) = 0;
};

#endif

