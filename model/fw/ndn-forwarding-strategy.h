/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil -*- */
/*
 * Copyright (c) 2011 University of California, Los Angeles
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author:  Alexander Afanasyev <alexander.afanasyev@ucla.edu>
 */

#ifndef NDN_FORWARDING_STRATEGY_H
#define NDN_FORWARDING_STRATEGY_H

#include "ns3/packet.h"
#include "ns3/callback.h"
#include "ns3/object.h"
#include "ns3/traced-callback.h"

namespace ns3 {
namespace ndn {

class Face;
class InterestHeader;
class ContentObjectHeader;
class Pit;
namespace pit { class Entry; }
class FibFaceMetric;
class Fib;
class ContentStore;

/**
 * \ingroup ndn
 * \brief Abstract base class for Ndn forwarding strategies
 */
class ForwardingStrategy :
    public Object
{
public:
  static TypeId GetTypeId (void);

  /**
   * @brief Default constructor
   */
  ForwardingStrategy ();
  virtual ~ForwardingStrategy ();

  /**
   * \brief Actual processing of incoming Ndn interests. Note, interests do not have payload
   * 
   * Processing Interest packets
   * @param face    incoming face
   * @param header  deserialized Interest header
   * @param origPacket  original packet
   */
  virtual void
  OnInterest (Ptr<Face> face,
              Ptr<const InterestHeader> header,
              Ptr<const Packet> origPacket);

  /**
   * \brief Actual processing of incoming Ndn content objects
   * 
   * Processing ContentObject packets
   * @param face    incoming face
   * @param header  deserialized ContentObject header
   * @param payload data packet payload
   * @param origPacket  original packet
   */
  virtual void
  OnData (Ptr<Face> face,
          Ptr<const ContentObjectHeader> header,
          Ptr<Packet> payload,
          Ptr<const Packet> origPacket);

  /**
   * @brief Event fired just before PIT entry is removed by timeout
   * @param pitEntry PIT entry to be removed
   */
  virtual void
  WillEraseTimedOutPendingInterest (Ptr<pit::Entry> pitEntry);

  /**
   * @brief Event fired every time face is added to NDN stack
   * @param face face to be removed
   */
  virtual void
  AddFace (Ptr<Face> face);
  
  /**
   * @brief Event fired every time face is removed from NDN stack
   * @param face face to be removed
   *
   * For example, when an application terminates, AppFace is removed and this method called by NDN stack.
   */
  virtual void
  RemoveFace (Ptr<Face> face);
  
protected:
  /**
   * @brief An event that is fired every time a new PIT entry is created
   *
   * Note that if NDN node is receiving a similar interest (interest for the same name),
   * then either DidReceiveDuplicateInterest, DidSuppressSimilarInterest, or DidForwardSimilarInterest
   * will be called
   * 
   * Suppression of similar Interests is controlled using ShouldSuppressIncomingInterest virtual method
   *
   * @param inFace  incoming face
   * @param header  deserialized Interest header
   * @param origPacket  original packet
   * @param pitEntry created PIT entry (incoming and outgoing face sets are empty)
   *
   * @see DidReceiveDuplicateInterest, DidSuppressSimilarInterest, DidForwardSimilarInterest, ShouldSuppressIncomingInterest
   */
  virtual void
  DidCreatePitEntry (Ptr<Face> inFace,
                     Ptr<const InterestHeader> header,
                     Ptr<const Packet> origPacket,
                     Ptr<pit::Entry> pitEntry);
  
  /**
   * @brief An event that is fired every time a new PIT entry cannot be created (e.g., PIT container imposes a limit)
   *
   * Note that this call can be called only for non-similar Interest (i.e., there is an attempt to create a new PIT entry).
   * For any non-similar Interests, either FailedToCreatePitEntry or DidCreatePitEntry is called.
   * 
   * @param inFace  incoming face
   * @param header  deserialized Interest header
   * @param origPacket  original packet
   */
  virtual void
  FailedToCreatePitEntry (Ptr<Face> inFace,
                          Ptr<const InterestHeader> header,
                          Ptr<const Packet> origPacket);
  
  /**
   * @brief An event that is fired every time a duplicated Interest is received
   *
   * This even is the last action that is performed before the Interest processing is halted
   *
   * @param inFace  incoming face
   * @param header  deserialized Interest header
   * @param origPacket  original packet
   * @param pitEntry an existing PIT entry, corresponding to the duplicated Interest
   *
   * @see DidReceiveDuplicateInterest, DidSuppressSimilarInterest, DidForwardSimilarInterest, ShouldSuppressIncomingInterest
   */
  virtual void
  DidReceiveDuplicateInterest (Ptr<Face> inFace,
                               Ptr<const InterestHeader> header,
                               Ptr<const Packet> origPacket,
                               Ptr<pit::Entry> pitEntry);

  /**
   * @brief An event that is fired every time when a similar Interest is received and suppressed (collapsed)
   *
   * This even is the last action that is performed before the Interest processing is halted
   *
   * @param inFace  incoming face
   * @param header  deserialized Interest header
   * @param origPacket  original packet
   * @param pitEntry an existing PIT entry, corresponding to the duplicated Interest
   *
   * @see DidReceiveDuplicateInterest, DidForwardSimilarInterest, ShouldSuppressIncomingInterest
   */
  virtual void
  DidSuppressSimilarInterest (Ptr<Face> inFace,
                              Ptr<const InterestHeader> header,
                              Ptr<const Packet> origPacket,
                              Ptr<pit::Entry> pitEntry);

  /**
   * @brief An event that is fired every time when a similar Interest is received and further forwarded (not suppressed/collapsed)
   *
   * This even is fired just before handling the Interest to PropagateInterest method
   *
   * @param inFace  incoming face
   * @param header  deserialized Interest header
   * @param origPacket  original packet
   * @param pitEntry an existing PIT entry, corresponding to the duplicated Interest
   *
   * @see DidReceiveDuplicateInterest, DidSuppressSimilarInterest, ShouldSuppressIncomingInterest
   */
  virtual void
  DidForwardSimilarInterest (Ptr<Face> inFace,
                             Ptr<const InterestHeader> header,
                             Ptr<const Packet> origPacket,
                             Ptr<pit::Entry> pitEntry);

  /**
   * @brief An even that is fired when Interest cannot be forwarded
   *
   * Note that the event will not fire if  retransmission detection is enabled (by default)
   * and retransmitted Interest cannot by forwarded.  For more details, refer to the implementation.
   *
   * @param inFace  incoming face
   * @param header  deserialized Interest header
   * @param origPacket  original packet
   * @param pitEntry an existing PIT entry, corresponding to the duplicated Interest
   *
   * @see DetectRetransmittedInterest
   */
  virtual void
  DidExhaustForwardingOptions (Ptr<Face> inFace,
                               Ptr<const InterestHeader> header,
                               Ptr<const Packet> origPacket,
                               Ptr<pit::Entry> pitEntry);

  virtual bool
  DetectRetransmittedInterest (Ptr<Face> inFace,
                               Ptr<const InterestHeader> header,
                               Ptr<const Packet> origPacket,
                               Ptr<pit::Entry> pitEntry);

  // When Interest is satisfied from the cache, incoming face is 0
  virtual void
  WillSatisfyPendingInterest (Ptr<Face> inFace,
                              Ptr<pit::Entry> pitEntry);

  // for data received both from network and cache
  virtual void
  SatisfyPendingInterest (Ptr<Face> inFace, // 0 allowed (from cache)
                          Ptr<const ContentObjectHeader> header,
                          Ptr<const Packet> payload,
                          Ptr<const Packet> origPacket,
                          Ptr<pit::Entry> pitEntry);

  virtual void
  DidSendOutData (Ptr<Face> inFace,
                  Ptr<const ContentObjectHeader> header,
                  Ptr<const Packet> payload,
                  Ptr<const Packet> origPacket,
                  Ptr<pit::Entry> pitEntry);
  
  virtual void
  DidReceiveUnsolicitedData (Ptr<Face> inFace,
                             Ptr<const ContentObjectHeader> header,
                             Ptr<const Packet> payload,
                             Ptr<const Packet> origPacket);
  
  virtual bool
  ShouldSuppressIncomingInterest (Ptr<Face> inFace,
                                  Ptr<const InterestHeader> header,
                                  Ptr<const Packet> origPacket,
                                  Ptr<pit::Entry> pitEntry);

  /**
   * @brief Event fired before actually sending out an interest
   *
   * If event returns false, then there is some kind of a problem (e.g., per-face limit reached)
   */
  virtual bool
  TrySendOutInterest (Ptr<Face> inFace,
                      Ptr<Face> outFace,
                      Ptr<const InterestHeader> header,
                      Ptr<const Packet> origPacket,
                      Ptr<pit::Entry> pitEntry);

  /**
   * @brief Event fired just after sending out an interest
   */
  virtual void
  DidSendOutInterest (Ptr<Face> outFace,
                      Ptr<const InterestHeader> header,
                      Ptr<const Packet> origPacket,
                      Ptr<pit::Entry> pitEntry);

  /**
   * @brief Wrapper method, which performs general tasks and calls DoPropagateInterest method
   *
   * General tasks so far are adding face to the list of incoming face, updating
   * PIT entry lifetime, calling DoPropagateInterest, and retransmissions (enabled by default).
   *
   * @param inFace     incoming face
   * @param header     Interest header
   * @param origPacket original Interest packet
   * @param pitEntry   reference to PIT entry (reference to corresponding FIB entry inside)
   *
   * @see DoPropagateInterest
   */
  virtual void
  PropagateInterest (Ptr<Face> inFace,
                     Ptr<const InterestHeader> header,
                     Ptr<const Packet> origPacket,
                     Ptr<pit::Entry> pitEntry);
  
  /**
   * @brief Virtual method to perform Interest propagation according to the forwarding strategy logic
   *
   * In most cases, this is the call that needs to be implemented/re-implemented in order
   * to perform forwarding of Interests according to the desired logic.
   *
   * There is also PropagateInterest method (generally, do not require to be overriden)
   * which performs general tasks (adding face to the list of incoming face, updating
   * PIT entry lifetime, calling DoPropagateInterest, as well as perform retransmissions (enabled by default).
   *
   * @param inFace     incoming face
   * @param header     Interest header
   * @param origPacket original Interest packet
   * @param pitEntry   reference to PIT entry (reference to corresponding FIB entry inside)
   *
   * @return true if interest was successfully propagated, false if all options have failed
   *
   * @see PropagateInterest
   */
  virtual bool
  DoPropagateInterest (Ptr<Face> inFace,
                       Ptr<const InterestHeader> header,
                       Ptr<const Packet> origPacket,
                       Ptr<pit::Entry> pitEntry) = 0;
  
protected:
  // inherited from Object class                                                                                                                                                        
  virtual void NotifyNewAggregate (); ///< @brief Even when object is aggregated to another Object
  virtual void DoDispose (); ///< @brief Do cleanup
  
protected:  
  Ptr<Pit> m_pit; ///< \brief Reference to PIT to which this forwarding strategy is associated
  Ptr<Fib> m_fib; ///< \brief FIB  
  Ptr<ContentStore> m_contentStore; ///< \brief Content store (for caching purposes only)

  bool m_cacheUnsolicitedData;
  bool m_detectRetransmissions;
  
  TracedCallback<Ptr<const InterestHeader>,
                 Ptr<const Face> > m_outInterests; ///< @brief Transmitted interests trace

  TracedCallback<Ptr<const InterestHeader>,
                 Ptr<const Face> > m_inInterests; ///< @brief trace of incoming Interests

  TracedCallback<Ptr<const InterestHeader>,
                 Ptr<const Face> > m_dropInterests; ///< @brief trace of dropped Interests
  
  ////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////
  ////////////////////////////////////////////////////////////////////

  TracedCallback<Ptr<const ContentObjectHeader>, Ptr<const Packet>,
                 bool /*from cache*/,
                 Ptr<const Face> > m_outData; ///< @brief trace of outgoing Data

  TracedCallback<Ptr<const ContentObjectHeader>, Ptr<const Packet>,
                 Ptr<const Face> > m_inData; ///< @brief trace of incoming Data

  TracedCallback<Ptr<const ContentObjectHeader>, Ptr<const Packet>,
                  Ptr<const Face> > m_dropData;  ///< @brief trace of dropped Data
};

} // namespace ndn
} // namespace ns3

#endif /* NDN_FORWARDING_STRATEGY_H */