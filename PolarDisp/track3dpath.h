#ifndef TRACK3DPATH_H
#define TRACK3DPATH_H

#include <algorithm>
#include <cstddef>
#include <vector>

struct Track3DPathSample
{
    double eastM;
    double northM;
    double heightM;
    float rangeM;
    unsigned rgb;
    unsigned targetRecResult = 0;
    bool offline = false;
};

class Track3DPath
{
public:
    int size() const { return static_cast<int>(m_samples.size()); }

    const Track3DPathSample& at(int index) const
    {
        return m_samples[(m_first + static_cast<std::size_t>(index)) % m_samples.size()];
    }

    Track3DPathSample& at(int index)
    {
        return m_samples[(m_first + static_cast<std::size_t>(index)) % m_samples.size()];
    }

    void append(const Track3DPathSample& sample, int limit)
    {
        if (size() < limit) {
            // A larger limit may reopen a previously full ring.
            normalize();
            m_samples.push_back(sample);
        } else {
            m_samples[m_first] = sample;
            m_first = (m_first + 1) % m_samples.size();
        }
    }

    void trim(int limit)
    {
        if (size() <= limit) return;
        std::vector<Track3DPathSample> tail;
        tail.reserve(static_cast<std::size_t>(limit));
        for (int index = size() - limit; index < size(); ++index) {
            tail.push_back(at(index));
        }
        m_samples.swap(tail);
        m_first = 0;
    }

private:
    void normalize()
    {
        if (m_first == 0) return;
        std::rotate(m_samples.begin(), m_samples.begin() + m_first, m_samples.end());
        m_first = 0;
    }

    std::vector<Track3DPathSample> m_samples;
    std::size_t m_first = 0;
};

#endif // TRACK3DPATH_H
