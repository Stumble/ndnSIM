/* -*- Mode:C++; c-file-style:"gnu"; indent-tabs-mode:nil; -*- */
/**
 * Copyright (c) 2011-2015  Regents of the University of California.
 *
 * This file is part of ndnSIM. See AUTHORS for complete list of ndnSIM authors and
 * contributors.
 *
 * ndnSIM is free software: you can redistribute it and/or modify it under the terms
 * of the GNU General Public License as published by the Free Software Foundation,
 * either version 3 of the License, or (at your option) any later version.
 *
 * ndnSIM is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE.  See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along with
 * ndnSIM, e.g., in COPYING.md file.  If not, see <http://www.gnu.org/licenses/>.
 **/

#include "helper/ndn-strategy-choice-helper.hpp"

#include "../tests-common.hpp"

namespace ns3 {
namespace ndn {

class StrategyChoiceHelperFixture : public ScenarioHelperWithCleanupFixture
{
public:
  StrategyChoiceHelperFixture()
  {
    Config::SetDefault("ns3::PointToPointNetDevice::DataRate", StringValue("10Mbps"));
    Config::SetDefault("ns3::PointToPointChannel::Delay", StringValue("1ms"));
    Config::SetDefault("ns3::DropTailQueue::MaxPackets", StringValue("500"));

    // Creating two 3 node topologies:                      //
    //                                                      //
    //                 +----+                     +----+    //
    //              +- | B1 |                  +- | B2 |    //
    //             /   +----+                 /   +----+    //
    //  +----+    /                +----+    /              //
    //  |    | --+                 |    | --+               //
    //  | A1 |                     | A2 |                   //
    //  |    | --+                 |    | --+               //
    //  +----+    \                +----+    \              //
    //             \   +----+                 \   +----+    //
    //              +- | C1 |                  +- | C2 |    //
    //                 +----+                     +----+    //

    createTopology({
        {"A1", "B1"},
        {"A1", "C1"},
        {"A2", "B2"},
        {"A2", "C2"}
      });

    addRoutes({
        {"A1", "B1", "/prefix", 200},
        {"A1", "C1", "/prefix", 100},
        {"A2", "B2", "/prefix", 100},
        {"A2", "C2", "/prefix", 200}
      });

    addApps({
        {"A1", "ns3::ndn::ConsumerCbr",
            {{"Prefix", "/prefix"}, {"Frequency", "1"}},
            "0s", "100s"},
        {"A2", "ns3::ndn::ConsumerCbr",
            {{"Prefix", "/prefix"}, {"Frequency", "1"}},
            "0s", "100s"},
      });
  }
};

BOOST_FIXTURE_TEST_SUITE(TestStrategyChoiceHelper, StrategyChoiceHelperFixture)

BOOST_AUTO_TEST_CASE(DefaultStrategies)
{
  Simulator::Stop(Seconds(5.0));
  Simulator::Run();

  BOOST_CHECK_EQUAL(getFace("A1", "B1")->getFaceStatus().getNOutInterests(), 0);
  BOOST_CHECK_EQUAL(getFace("A1", "C1")->getFaceStatus().getNOutInterests(), 5);

  BOOST_CHECK_EQUAL(getFace("A2", "B2")->getFaceStatus().getNOutInterests(), 5);
  BOOST_CHECK_EQUAL(getFace("A2", "C2")->getFaceStatus().getNOutInterests(), 0);
}

// static void
// Install(Ptr<Node> node, const Name& namePrefix, const Name& strategy);
BOOST_AUTO_TEST_CASE(InstallBuiltInStrategyOnNode)
{
  StrategyChoiceHelper::Install(getNode("A2"), "/prefix", "/localhost/nfd/strategy/broadcast");

  Simulator::Stop(Seconds(5.0));
  Simulator::Run();

  BOOST_CHECK_EQUAL(getFace("A1", "B1")->getFaceStatus().getNOutInterests(), 0);
  BOOST_CHECK_EQUAL(getFace("A1", "C1")->getFaceStatus().getNOutInterests(), 5);

  BOOST_CHECK_EQUAL(getFace("A2", "B2")->getFaceStatus().getNOutInterests(), 5);
  BOOST_CHECK_EQUAL(getFace("A2", "C2")->getFaceStatus().getNOutInterests(), 5);
}

// static void
// Install(Ptr<Node> node, const Name& namePrefix, const Name& strategy);
BOOST_AUTO_TEST_CASE(InstallBuiltInStrategyOnNodeContainer)
{
  NodeContainer nodes;
  nodes.Add(getNode("A1"));
  nodes.Add(getNode("A2"));

  StrategyChoiceHelper::Install(nodes, "/prefix", "/localhost/nfd/strategy/broadcast");

  Simulator::Stop(Seconds(5.0));
  Simulator::Run();

  BOOST_CHECK_EQUAL(getFace("A1", "B1")->getFaceStatus().getNOutInterests(), 5);
  BOOST_CHECK_EQUAL(getFace("A1", "C1")->getFaceStatus().getNOutInterests(), 5);

  BOOST_CHECK_EQUAL(getFace("A2", "B2")->getFaceStatus().getNOutInterests(), 5);
  BOOST_CHECK_EQUAL(getFace("A2", "C2")->getFaceStatus().getNOutInterests(), 5);
}

// static void
// InstallAll(const Name& namePrefix, const Name& strategy);
BOOST_AUTO_TEST_CASE(InstallAllBuiltInStrategy)
{
  StrategyChoiceHelper::InstallAll("/prefix", "/localhost/nfd/strategy/broadcast");

  Simulator::Stop(Seconds(5.0));
  Simulator::Run();

  BOOST_CHECK_EQUAL(getFace("A1", "B1")->getFaceStatus().getNOutInterests(), 5);
  BOOST_CHECK_EQUAL(getFace("A1", "C1")->getFaceStatus().getNOutInterests(), 5);

  BOOST_CHECK_EQUAL(getFace("A2", "B2")->getFaceStatus().getNOutInterests(), 5);
  BOOST_CHECK_EQUAL(getFace("A2", "C2")->getFaceStatus().getNOutInterests(), 5);
}


class NullStrategy : public nfd::fw::Strategy {
public:
  NullStrategy(nfd::Forwarder& forwarder)
    : Strategy(forwarder, STRATEGY_NAME)
  {
  }

  virtual void
  afterReceiveInterest(const Face& inFace, const Interest& interest,
                       shared_ptr<nfd::fib::Entry> fibEntry, shared_ptr<nfd::pit::Entry> pitEntry)
  {
    // this strategy doesn't forward interests
  }

public:
  static const Name STRATEGY_NAME;
};

const Name NullStrategy::STRATEGY_NAME = "ndn:/localhost/nfd/strategy/unit-tests/null-strategy";

// template<class Strategy>
// static void
// Install(Ptr<Node> node, const Name& namePrefix);
BOOST_AUTO_TEST_CASE(InstallCustomStrategyOnNode)
{
  StrategyChoiceHelper::Install<NullStrategy>(getNode("A2"), "/prefix");

  Simulator::Stop(Seconds(5.0));
  Simulator::Run();

  BOOST_CHECK_EQUAL(getFace("A1", "B1")->getFaceStatus().getNOutInterests(), 0);
  BOOST_CHECK_EQUAL(getFace("A1", "C1")->getFaceStatus().getNOutInterests(), 5);

  BOOST_CHECK_EQUAL(getFace("A2", "B2")->getFaceStatus().getNOutInterests(), 0);
  BOOST_CHECK_EQUAL(getFace("A2", "C2")->getFaceStatus().getNOutInterests(), 0);
}

// template<class Strategy>
// static void
// Install(const NodeContainer& c, const Name& namePrefix);
BOOST_AUTO_TEST_CASE(InstallCustomStrategyOnNodeContainer)
{
  NodeContainer nodes;
  nodes.Add(getNode("A1"));
  nodes.Add(getNode("A2"));

  StrategyChoiceHelper::Install<NullStrategy>(nodes, "/prefix");

  Simulator::Stop(Seconds(5.0));
  Simulator::Run();

  BOOST_CHECK_EQUAL(getFace("A1", "B1")->getFaceStatus().getNOutInterests(), 0);
  BOOST_CHECK_EQUAL(getFace("A1", "C1")->getFaceStatus().getNOutInterests(), 0);

  BOOST_CHECK_EQUAL(getFace("A2", "B2")->getFaceStatus().getNOutInterests(), 0);
  BOOST_CHECK_EQUAL(getFace("A2", "C2")->getFaceStatus().getNOutInterests(), 0);
}

// template<class Strategy>
// static void
// InstallAll(const Name& namePrefix);
BOOST_AUTO_TEST_CASE(InstallAllCustomStrategy)
{
  StrategyChoiceHelper::InstallAll<NullStrategy>("/prefix");

  Simulator::Stop(Seconds(5.0));
  Simulator::Run();

  BOOST_CHECK_EQUAL(getFace("A1", "B1")->getFaceStatus().getNOutInterests(), 0);
  BOOST_CHECK_EQUAL(getFace("A1", "C1")->getFaceStatus().getNOutInterests(), 0);

  BOOST_CHECK_EQUAL(getFace("A2", "B2")->getFaceStatus().getNOutInterests(), 0);
  BOOST_CHECK_EQUAL(getFace("A2", "C2")->getFaceStatus().getNOutInterests(), 0);
}

BOOST_AUTO_TEST_SUITE_END()

} // namespace ndn
} // namespace ns3
