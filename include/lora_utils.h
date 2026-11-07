/* Copyright (C) 2025 Ricardo Guzman - CA2RXU
 * 
 * This file is part of LoRa APRS Tracker.
 * 
 * LoRa APRS Tracker is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or 
 * (at your option) any later version.
 * 
 * LoRa APRS Tracker is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with LoRa APRS Tracker. If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef LORA_UTILS_H_
#define LORA_UTILS_H_

#include <Arduino.h>
#include "relay_frame.h"

struct ReceivedLoRaPacket {
    String  text;
    int     rssi;
    float   snr;
    int     freqError;
};

struct DataRateConfig {
    int dataRate;
    int spreadingFactor;
    int codingRate4;
    long signalBandwidth;
};

// Flags externes pour les changements de configuration pendants
extern bool pendingFrequencyChange;
extern int pendingLoraIndex;
extern bool pendingDataRateChange;
extern int pendingDataRate;

namespace LoRa_Utils {

    void setFlag();
    void requestFrequencyChange(int newLoraIndex);
    void requestDataRateChange(int newDataRate);
    void processPendingChanges();
    int calculateDataRate(int sf, int cr, int bw);
    void changeFreq();
    void applyLoraConfig();
    void changeDataRate();
    void setDataRate(int dataRate);
    DataRateConfig getDataRateConfig(int dataRate);
    int getNextDataRate(int currentDataRate);
    void setup();
    void sendNewPacket(const String& newPacket, const RelayFrame* relay = nullptr);   // relay: the LACF frame to send instead of newPacket (a relayed LACF frame)
    // A packet the digipeater has just rewritten (ownCall* in its path), from the frame received last: sent as an LACF frame that keeps the earlier hops' tuples and carries the relay's own
    // (spec 11, "Relay"), with rssi, snr and freqError of that reception. When it cannot be relayed that way, or the Tx frame format is not hybrid, it is sent as sendNewPacket would.
    void sendRelayedPacket(const String& newPacket, const String& ownCall, int rssi, float snr, int freqError);
    // A 3C FF 03 frame this station could not decode (a model or destination code it does not hold) but whose structure and CRC are good (3.0.0-dev.1, spec 12, "Relay without the model"):
    // when the repeater or digipeater is on and the Tx frame format is hybrid, the digipeater's own rule (APRSPacketLib::generateDigipeatedPacket) is applied to its front and the frame is relayed
    // with that path, its content copied as received. It is never shown, stored or sent as text; a frame that cannot be relayed as a frame is dropped. Call after receivePacket().
    void handleRelayOnly(const String& ownCall, const String& digipeatAlias, bool repeaterMode, bool digipeaterActive);
    void wakeRadio();
    ReceivedLoRaPacket receiveFromSleep();
    ReceivedLoRaPacket receivePacket();
    void sleepRadio();

}

#endif