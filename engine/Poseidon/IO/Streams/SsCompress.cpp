#include <Poseidon/IO/Streams/QStream.hpp>
#include <Poseidon/Foundation/Framework/DebugLog.hpp>
#include <Poseidon/Foundation/Types/Memtype.h>

namespace Poseidon
{
bool SSCompress::Decode(char* dst, long lensb, QIStream& in)
{
    if (lensb <= 0)
    {
        return true;
    }

    int i, j, r, c, csum = 0, csr;
    int flags;
    for (i = 0; i < N - F; i++)
    {
        text_buf[i] = ' ';
    }
    r = N - F;
    flags = 0;
    while (lensb > 0)
    {
        if (((flags >>= 1) & 256) == 0)
        {
            c = in.get();
            flags = c | 0xff00;
        }
        if (in.fail() || in.eof())
        {
            Fail("LZW: stream read failed");
            return false;
        }
        if (flags & 1)
        {
            c = in.get();
            if (in.fail() || in.eof())
            {
                Fail("LZW: stream read failed");
                return false;
            }
            csum += (unsigned char)c;
            *dst++ = c;
            lensb--;
            text_buf[r] = (unsigned char)c;
            r++;
            r &= (N - 1);
        }
        else
        {
            i = in.get();
            j = in.get();
            if (in.fail() || in.eof())
            {
                Fail("LZW: stream read failed");
                return false;
            }
            i |= (j & 0xf0) << 4;
            j &= 0x0f;
            j += THRESHOLD;
            // Stop at lensb: a back-reference match copies up to F+THRESHOLD bytes in
            // one step, but only `lensb` output bytes were requested (and dst is sized
            // for exactly that). A crafted stream whose final match runs past the end
            // would otherwise overflow the output buffer.
            for (i = r - i, j += i; i <= j && lensb > 0; i++)
            {
                c = (byte)text_buf[i & (N - 1)];
                csum += (unsigned char)c;
                *dst++ = c;
                lensb--;
                text_buf[r] = (unsigned char)c;
                r++;
                r &= (N - 1);
            }
        }
    }
    in.read(&csr, sizeof(csr));
    if (in.fail() || in.eof())
    {
        Fail("LZW: end of stream");
        return false;
    }
    if (csr != csum)
    {
        Fail("Checksum");
        return false;
    }
    return true;
}

#define lson(n) (lsons[n])
#define rson(n) (rsons[n])
#define dad(n) (dads[n])
#define NIL N

void SSCompress::InitTree()
{
    int i;

    for (i = N + 1; i <= N + 256; i++)
    {
        rsons[i] = NIL;
    }
    for (i = 0; i < N; i++)
    {
        dads[i] = NIL;
    }
}

void SSCompress::InsertNode(int r)
{
    int i, p, cmp;
    byte* key;

    cmp = 1;
    key = &text_buf[r];
    p = (N + 1 + key[0]);
    rson(r) = lson(r) = NIL;
    match_len = 0;
    for (;;)
    {
        if (cmp)
        {
            if (rson(p) != NIL)
            {
                p = rson(p);
            }
            else
            {
                rson(p) = r;
                dad(r) = p;
                return;
            }
        }
        else
        {
            if (lson(p) != NIL)
            {
                p = lson(p);
            }
            else
            {
                lson(p) = r;
                dad(r) = p;
                return;
            }
        }
        {
            byte* tbp = &text_buf[p + 1];
            byte* kp = &key[1];
            for (i = 1; i < F; i++)
            {
                if (*kp++ != *tbp++)
                {
                    cmp = kp[-1] >= tbp[-1];
                    break;
                }
            }
            if (i > match_len)
            {
                match_position = p;
                if ((match_len = i) >= F)
                {
                    break;
                }
            }
        }
    }
    dad(r) = dad(p);
    lson(r) = lson(p);
    rson(r) = rson(p);
    dad(lson(p)) = r;
    dad(rson(p)) = r;
    if (rson(dad(p)) == p)
    {
        rson(dad(p)) = r;
    }
    else
    {
        lson(dad(p)) = r;
    }
    dad(p) = NIL; /* remove p */
}

void SSCompress::DeleteNode(int p)
{
    int q;
    if (dad(p) == NIL)
    {
        return; /* not in tree */
    }
    if (rson(p) == NIL)
    {
        q = lson(p);
    }
    else if (lson(p) == NIL)
    {
        q = rson(p);
    }
    else
    {
        q = lson(p);
        if (rson(q) != NIL)
        {
            do
            {
                q = rson(q);
            } while (rson(q) != NIL);
            rson(dad(q)) = lson(q);
            dad(lson(q)) = dad(q);
            lson(q) = lson(p);
            dad(lson(p)) = q;
        }
        rson(q) = rson(p);
        dad(rson(p)) = q;
    }
    dad(q) = dad(p);
    if (rson(dad(p)) == p)
    {
        rson(dad(p)) = q;
    }
    else
    {
        lson(dad(p)) = q;
    }
    dad(p) = NIL;
}

#define NextByte(data, lensb) (--lensb >= 0 ? (unsigned char)*data++ : -1)

void SSCompress::Encode(QOStream& out, const char* data, long lensb)
{
    int i, c, len, r, s, last_match_len, CPtr;
    unsigned char CBuf[17];
    unsigned char mask;
    int textsize;
    int csum;
    if (lensb == 0)
    {
        return;
    }
    textsize = 0;
    InitTree();
    csum = 0;
    CBuf[0] = 0;
    CPtr = mask = 1;
    s = 0;
    r = N - F;
    long origLen = lensb;
    for (i = s; i < r; i++)
    {
        text_buf[i] = ' ';
    }
    for (len = 0; len < F && (c = NextByte(data, lensb)) >= 0; len++)
    {
        text_buf[r + len] = (unsigned char)c;
        csum += (unsigned char)c;
    }
    textsize = len;
    PoseidonAssert(textsize != 0);
    for (i = 1; i <= F; i++)
    {
        InsertNode(r - i);
    }
    InsertNode(r);
    do
    {
        if (match_len > len)
        {
            match_len = len;
        }
        if (match_len <= THRESHOLD)
        {
            match_len = 1;
            CBuf[0] |= mask;
            CBuf[CPtr++] = text_buf[r];
        }
        else
        {
            int mp = (r - match_position) & (N - 1);
            CBuf[CPtr++] = (unsigned char)mp;
            CBuf[CPtr++] = (unsigned char)(((mp >> 4) & 0xf0) | (match_len - (THRESHOLD + 1)));
        }
        if ((mask <<= 1) == 0)
        {
            out.write(CBuf, CPtr);
            CBuf[0] = 0;
            CPtr = mask = 1;
        }
        last_match_len = match_len;
        for (i = 0; i < last_match_len && (c = NextByte(data, lensb)) >= 0; i++)
        {
            DeleteNode(s); /* Delete old strings and read new chars */
            text_buf[s] = (unsigned char)c;
            csum += (unsigned char)c;
            if (s < F - 1)
            {
                text_buf[s + N] = (unsigned char)c; /* beg. of buf. */
            }
            s++;
            s &= N - 1;
            r++;
            r &= N - 1;
            InsertNode(r);
        }
        textsize += i;
        while (i++ < last_match_len)
        {
            DeleteNode(s); /* EOF => no need to read, but */
            s++;
            s &= N - 1;
            r++;
            r &= N - 1;
            if (--len)
            {
                InsertNode(r); /* buffer may not be empty. */
            }
        }
    } while (len > 0);
    PoseidonAssert(textsize == origLen);
    if (CPtr > 1)
    {
        out.write(CBuf, CPtr);
    }
    out.write(&csum, sizeof(csum));
    PoseidonAssert(lensb <= 0);
}

} // namespace Poseidon
