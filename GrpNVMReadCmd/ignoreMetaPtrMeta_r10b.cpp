/*
 * Copyright (c) 2011, Intel Corporation.
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <boost/format.hpp>
#include "ignoreMetaPtrMeta_r10b.h"
#include "globals.h"
#include "grpDefs.h"
#include "../Queues/iocq.h"
#include "../Queues/iosq.h"
#include "../Utils/io.h"
#include "../Cmds/read.h"

namespace GrpNVMReadCmd {


IgnoreMetaPtrMeta_r10b::IgnoreMetaPtrMeta_r10b(int fd, string mGrpName,
    string mTestName, ErrorRegs errRegs) :
    Test(fd, mGrpName, mTestName, SPECREV_10b, errRegs)
{
    // 63 chars allowed:     xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
    mTestDesc.SetCompliance("revision 1.0b, section 4,6");
    mTestDesc.SetShort(     "Verify metadata ptr is not used for meta namspc");
    // No string size limit for the long description
    mTestDesc.SetLong(
        "MPTR is only used if metadata is not interleaved with the data. "
        "For all meta namspcs from Identify.NN with Idenitfy.FLBAS_b4 = 1, "
        "issue a single read cmd and approp metadata requirements requesting 1 "
        "data block at LBA 0; set the meta ptr to max value, expect success.");
}


IgnoreMetaPtrMeta_r10b::~IgnoreMetaPtrMeta_r10b()
{
    ///////////////////////////////////////////////////////////////////////////
    // Allocations taken from the heap and not under the control of the
    // RsrcMngr need to be freed/deleted here.
    ///////////////////////////////////////////////////////////////////////////
}


IgnoreMetaPtrMeta_r10b::
IgnoreMetaPtrMeta_r10b(const IgnoreMetaPtrMeta_r10b &other) : Test(other)
{
    ///////////////////////////////////////////////////////////////////////////
    // All pointers in this object must be NULL, never allow shallow or deep
    // copies, see Test::Clone() header comment.
    ///////////////////////////////////////////////////////////////////////////
}


IgnoreMetaPtrMeta_r10b &
IgnoreMetaPtrMeta_r10b::operator=(const IgnoreMetaPtrMeta_r10b &other)
{
    ///////////////////////////////////////////////////////////////////////////
    // All pointers in this object must be NULL, never allow shallow or deep
    // copies, see Test::Clone() header comment.
    ///////////////////////////////////////////////////////////////////////////
    Test::operator=(other);
    return *this;
}


void
IgnoreMetaPtrMeta_r10b::RunCoreTest()
{
    /** \verbatim
     * Assumptions:
     * 1) Test CreateResources_r10b has run prior.
     * \endverbatim
     */
    string work;
    ConstSharedIdentifyPtr namSpcPtr;

    LOG_NRM("Lookup objs which were created in a prior test within group");
    SharedIOSQPtr iosq = CAST_TO_IOSQ(gRsrcMngr->GetObj(IOSQ_GROUP_ID));
    SharedIOCQPtr iocq = CAST_TO_IOCQ(gRsrcMngr->GetObj(IOCQ_GROUP_ID));

    LOG_NRM("For all imeta namspc's issue read cmd with non-zero meta ptr");
    vector<uint32_t> imeta = gInformative->GetMetaINamespaces();
    for (size_t i = 0; i < imeta.size(); i++) {
        namSpcPtr = gInformative->GetIdentifyCmdNamspc(imeta[i]);

        LOG_NRM("Setup read cmd's values that won't change per namspc");
        SharedMemBufferPtr readMem = SharedMemBufferPtr(new MemBuffer());
        uint64_t lbaDataSize = namSpcPtr->GetLBADataSize();
        LBAFormat lbaFormat = namSpcPtr->GetLBAFormat();
        readMem->Init(lbaDataSize + lbaFormat.MS);

        SharedReadPtr readCmd = SharedReadPtr(new Read());
        send_64b_bitmask prpBitmask = (send_64b_bitmask)
            (MASK_PRP1_PAGE | MASK_PRP2_PAGE | MASK_PRP2_LIST);
        readCmd->SetPrpBuffer(prpBitmask, readMem);
        readCmd->SetNLB(0);    // convert to 0-based value

        LOG_NRM("Set MPTR in cmd to max value");
        readCmd->SetDword(0xffffffff, 4);
        readCmd->SetDword(0xffffffff, 5);

        readCmd->SetNSID(imeta[i]);
        work = str(boost::format("namspc%d") % imeta[i]);
        IO::SendAndReapCmd(mGrpName, mTestName, DEFAULT_CMD_WAIT_ms, iosq, iocq,
            readCmd, work, true);
    }
}


}   // namespace

