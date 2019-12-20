// Copyright (c) Microsoft Corporation. All rights reserved.
// Licensed under the MIT License.

#ifndef _SPTAG_COMMON_WORKSPACE_H_
#define _SPTAG_COMMON_WORKSPACE_H_

#include "CommonUtils.h"
#include "Heap.h"

namespace SPTAG
{
    namespace COMMON
    {
        // node type in the priority queue
        struct HeapCell
        {
            SizeType node;
            float distance;

            HeapCell(SizeType _node = -1, float _distance = MaxDist) : node(_node), distance(_distance) {}

            inline bool operator < (const HeapCell& rhs)
            {
                return distance < rhs.distance;
            }

            inline bool operator > (const HeapCell& rhs)
            {
                return distance > rhs.distance;
            }
        };

        class OptHashPosVector
        {
        protected:
            // Max loop number in one hash block.
            static const int m_maxLoop = 8;

            // Could we use the second hash block.
            bool m_secondHash;

            // Max pool size.
            int m_poolSize;

            // Record 2 hash tables.
            // [0~m_poolSize + 1) is the first block.
            // [m_poolSize + 1, 2*(m_poolSize + 1)) is the second block;
            std::unique_ptr<SizeType[]> m_hashTable;


            inline unsigned hash_func2(unsigned idx, int loop)
            {
                return (idx + loop) & m_poolSize;
            }


            inline unsigned hash_func(unsigned idx)
            {
                return ((unsigned)(idx * 99991) + _rotl(idx, 2) + 101) & m_poolSize;
            }

        public:
            OptHashPosVector() {}

            ~OptHashPosVector() {}


            void Init(SizeType size)
            {
                int ex = 0;
                while (size != 0) {
                    ex++;
                    size >>= 1;
                }
                m_secondHash = true;
                m_poolSize = (1 << (ex + 1)) - 1;
                m_hashTable.reset(new SizeType[(m_poolSize + 1) * 2]);
                clear();
            }

            void clear()
            {
                if (!m_secondHash)
                {
                    // Clear first block.
                    memset(m_hashTable.get(), 0, sizeof(SizeType) * (m_poolSize + 1));
                }
                else
                {
                    // Clear all blocks.
                    m_secondHash = false;
                    memset(m_hashTable.get(), 0, 2 * sizeof(SizeType) * (m_poolSize + 1));
                }
            }


            inline bool CheckAndSet(SizeType idx)
            {
                // Inner Index is begin from 1
                return _CheckAndSet(m_hashTable.get(), idx + 1) == 0;
            }


            inline int _CheckAndSet(SizeType* hashTable, SizeType idx)
            {
                unsigned index = hash_func((unsigned)idx);
                for (int loop = 0; loop < m_maxLoop; ++loop)
                {
                    if (!hashTable[index])
                    {
                        // index first match and record it.
                        hashTable[index] = idx;
                        return 1;
                    }
                    if (hashTable[index] == idx)
                    {
                        // Hit this item in hash table.
                        return 0;
                    }
                    // Get next hash position.
                    index = hash_func2(index, loop);
                }

                if (hashTable == m_hashTable.get())
                {
                    // Use second hash block.
                    m_secondHash = true;
                    return _CheckAndSet(m_hashTable.get() + m_poolSize + 1, idx);
                }

                // Do not include this item.
                return -1;
            }
        };

        // Variables for each single NN search
        struct WorkSpace
        {
            void Initialize(int maxCheck, SizeType dataSize)
            {
                nodeCheckStatus.Init(maxCheck);
                m_SPTQueue.Resize(maxCheck * 10);
                m_NGQueue.Resize(maxCheck * 30);

                m_iNumberOfTreeCheckedLeaves = 0;
                m_iNumberOfCheckedLeaves = 0;
                m_iContinuousLimit = maxCheck / 64;
                m_iMaxCheck = maxCheck;
                m_iNumOfContinuousNoBetterPropagation = 0;
            }

            void Reset(int maxCheck)
            {
                nodeCheckStatus.clear();
                m_SPTQueue.clear();
                m_NGQueue.clear();

                m_iNumOfContinuousNoBetterPropagation = 0;
                m_iContinuousLimit = maxCheck / 64;
                m_iNumberOfTreeCheckedLeaves = 0;
                m_iNumberOfCheckedLeaves = 0;
                m_iMaxCheck = maxCheck;
            }

            inline bool CheckAndSet(SizeType idx)
            {
                return nodeCheckStatus.CheckAndSet(idx);
            }

            OptHashPosVector nodeCheckStatus;

            // counter for dynamic pivoting
            int m_iNumOfContinuousNoBetterPropagation;
            int m_iContinuousLimit;
            int m_iNumberOfTreeCheckedLeaves;
            int m_iNumberOfCheckedLeaves;
            int m_iMaxCheck;

            // Prioriy queue used for neighborhood graph
            Heap<HeapCell> m_NGQueue;

            // Priority queue Used for Tree
            Heap<HeapCell> m_SPTQueue;
        };
    }
}

#endif // _SPTAG_COMMON_WORKSPACE_H_
