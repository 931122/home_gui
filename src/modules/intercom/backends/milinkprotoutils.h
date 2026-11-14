#ifndef MILINKPROTOUTILS_H
#define MILINKPROTOUTILS_H

#include <QByteArray>
#include <QList>
#include <QMap>
#include <QRandomGenerator>
#include <QString>
#include <QtEndian>

#if defined(MILINK_USE_APPLE_CRYPTO)
#include <CommonCrypto/CommonCryptor.h>
#include <CoreFoundation/CoreFoundation.h>
#include <Security/Security.h>
#include <algorithm>
#include <cstring>
#else
#include <openssl/evp.h>
#include <openssl/rsa.h>
#include <openssl/x509.h>
#endif
#include <zlib.h>

namespace MilinkProto {

inline constexpr quint32 MILINK_APP_ID = 10018;

inline constexpr quint8 MILINK_FIXED_IV[16] = {
    100, 23, 84, 114, 72, 0, 4, 97, 73, 97, 2, 52, 84, 102, 18, 32
};

inline QByteArray fixedIvBytes()
{
    return QByteArray(reinterpret_cast<const char *>(MILINK_FIXED_IV), sizeof(MILINK_FIXED_IV));
}

inline QByteArray handshakeTokenBytes()
{
    return QByteArrayLiteral("bsJ0RccDL4JvKAR660A6wzHXxRKRXWPBMowLR4m7mWg=");
}

inline QByteArray handshakeKeyBytes()
{
    return QByteArrayLiteral("4N9FcL47REBDdCHL");
}

inline const QByteArray &rsaPublicKeyB64(int index)
{
    static const QByteArray kKeys[4] = {
        QByteArrayLiteral("MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQCyyJogBGwLOXYp71IhCQuS0dqkrT0B62qat/P2kB/vdIC+eS0egYcGIvh73VQB6nyc4/jb5pdLeMyImf64EIwIfMqa6iWl0MBI9oXwjs9Dct70PE8FvtcwjUz/1ts0nBF/0vkll8f2W5XlcciEHuXBvBjOcshL866/w3f+Bg5woQIDAQAB"),
        QByteArrayLiteral("MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDH6Tvnsh5NM6WBzRqkHS5pyijmQ/W5LaL41CS7UGFNZlsl7/dke9Rt8tErcjzydbQ+fbXMD8dw36yIV64Q7CSkWr/qmy69/wBuijWWX4evFe557y5xm8GjhAPu4Yjz8TidqbI2H2EzSEjFltmSx2gpxEts//ifjLcMKhR43HSIKwIDAQAB"),
        QByteArrayLiteral("MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQDp9WJz0gTAQP5oneVMs+ubmBDn7uFbNJOetNH19Ze+4EyYxyg7zPr9rGlWuNmiQzd7KeHB6uOOBTOIYtRl0J/z4fx5aPejleHfJEd8urjkMCEReDTLKrFa0IeBHRqy8FyGjMtVQJUw9V30Jvy7WFFB07rTw1Lt2mJsQOpPt4CjPwIDAQAB"),
        QByteArrayLiteral("MIGfMA0GCSqGSIb3DQEBAQUAA4GNADCBiQKBgQD+qNU/W2iWBi5APoJ9nOSgD1IFCI18OQ6ksWDqjWK0GIpOU0wapEa9cVKbbhDkGwpX5I5JEuHygPsAEMWLRF6zr9h5RqdOjISlaeAU4nwsd4dJRNHeHON5COkGo38Eu9PJSGzOed7sjCC7XxCI+E2N7hiaFRQlF2obHQch6Cnb9wIDAQAB")
    };
    return kKeys[qBound(0, index, 3)];
}

enum EncodeType : quint8 {
    EncodeNone = 0,
    EncodeGcmMns = 2,
    EncodeCbcFastLogin = 3,
    EncodeGcmHandshake = 5,
    EncodeGcmType8 = 8,
    EncodeRsaChannel = 9,
    EncodeGcmDeviceLogin = 10
};

enum SignalAction : quint32 {
    ActionInvite = 1,
    ActionBusy = 2,
    ActionRing = 3,
    ActionAccept = 4,
    ActionCancel = 5,
    ActionPushAck = 7,
    ActionEventNotify = 8,
    ActionGMemberIsAlive = 10,
    ActionKick = 11
};

inline QByteArray randomAsciiKey(int length = 16)
{
    static constexpr char kAlphabet[] = "abcdefghijklmnopqrstuvwxy";
    QByteArray out(length, '\0');
    for (int i = 0; i < length; ++i) {
        out[i] = kAlphabet[QRandomGenerator::global()->bounded(static_cast<int>(sizeof(kAlphabet) - 1))];
    }
    return out;
}

namespace Crypto {

#if defined(MILINK_USE_APPLE_CRYPTO)

namespace detail {

inline void gf128Mul(const quint8 x[16], const quint8 y[16], quint8 out[16])
{
    quint8 z[16] = {0};
    quint8 v[16];
    std::memcpy(v, y, 16);

    for (int i = 0; i < 16; ++i) {
        for (int b = 7; b >= 0; --b) {
            if ((x[i] >> b) & 1) {
                for (int k = 0; k < 16; ++k) z[k] ^= v[k];
            }
            const bool lsb = (v[15] & 1) != 0;
            for (int k = 15; k > 0; --k) {
                v[k] = static_cast<quint8>((v[k] >> 1) | ((v[k - 1] & 1) << 7));
            }
            v[0] >>= 1;
            if (lsb) v[0] ^= 0xE1;
        }
    }
    std::memcpy(out, z, 16);
}

inline bool aes128EcbEncryptBlock(const quint8 key[16], const quint8 inBlock[16], quint8 outBlock[16])
{
    size_t moved = 0;
    const CCCryptorStatus st = CCCrypt(kCCEncrypt, kCCAlgorithmAES, kCCOptionECBMode,
                                       key, 16, nullptr,
                                       inBlock, 16, outBlock, 16, &moved);
    return st == kCCSuccess && moved == 16;
}

inline void ghashCompute(const quint8 h[16], const quint8 *cipher, int len, quint8 yOut[16])
{
    std::memset(yOut, 0, 16);
    for (int off = 0; off < len; off += 16) {
        quint8 blk[16] = {0};
        const int n = std::min(16, len - off);
        std::memcpy(blk, cipher + off, static_cast<size_t>(n));
        for (int k = 0; k < 16; ++k) yOut[k] ^= blk[k];
        quint8 next[16];
        gf128Mul(yOut, h, next);
        std::memcpy(yOut, next, 16);
    }
    quint8 lenBlk[16] = {0};
    const quint64 bits = static_cast<quint64>(len) * 8ULL;
    for (int i = 0; i < 8; ++i) {
        lenBlk[15 - i] = static_cast<quint8>((bits >> (i * 8)) & 0xFF);
    }
    for (int k = 0; k < 16; ++k) yOut[k] ^= lenBlk[k];
    quint8 next[16];
    gf128Mul(yOut, h, next);
    std::memcpy(yOut, next, 16);
}

inline void gcmInc32(quint8 block[16])
{
    quint32 ctr = (static_cast<quint32>(block[12]) << 24) |
                  (static_cast<quint32>(block[13]) << 16) |
                  (static_cast<quint32>(block[14]) << 8) |
                  static_cast<quint32>(block[15]);
    ++ctr;
    block[12] = static_cast<quint8>((ctr >> 24) & 0xFF);
    block[13] = static_cast<quint8>((ctr >> 16) & 0xFF);
    block[14] = static_cast<quint8>((ctr >> 8) & 0xFF);
    block[15] = static_cast<quint8>(ctr & 0xFF);
}

inline bool gcmPrepareState(const quint8 keyBytes[16], quint8 hOut[16], quint8 j0Out[16], quint8 encJ0Out[16])
{
    const quint8 zero[16] = {0};
    if (!aes128EcbEncryptBlock(keyBytes, zero, hOut)) return false;
    ghashCompute(hOut, MILINK_FIXED_IV, sizeof(MILINK_FIXED_IV), j0Out);
    return aes128EcbEncryptBlock(keyBytes, j0Out, encJ0Out);
}

inline bool gcmXorStream(const quint8 keyBytes[16], const quint8 j0[16], const quint8 *inPtr, int len, quint8 *outPtr)
{
    quint8 cb[16];
    std::memcpy(cb, j0, 16);
    for (int off = 0; off < len; off += 16) {
        gcmInc32(cb);
        quint8 ks[16];
        if (!aes128EcbEncryptBlock(keyBytes, cb, ks)) return false;
        const int n = std::min(16, len - off);
        for (int i = 0; i < n; ++i) {
            outPtr[off + i] = inPtr[off + i] ^ ks[i];
        }
    }
    return true;
}

inline bool readAsn1Length(const quint8 *buf, int total, int &pos, int &lenOut)
{
    if (pos >= total) return false;
    const quint8 b = buf[pos++];
    if ((b & 0x80) == 0) {
        lenOut = b;
        return pos + lenOut <= total;
    }
    const int num = b & 0x7F;
    if (num == 0 || num > 4 || pos + num > total) return false;
    lenOut = 0;
    for (int i = 0; i < num; ++i) {
        lenOut = (lenOut << 8) | buf[pos++];
    }
    return lenOut >= 0 && pos + lenOut <= total;
}

inline QByteArray spkiDerToPkcs1Der(const QByteArray &spki)
{
    const quint8 *buf = reinterpret_cast<const quint8 *>(spki.constData());
    const int total = spki.size();
    int pos = 0;
    int len = 0;
    if (pos >= total || buf[pos++] != 0x30 || !readAsn1Length(buf, total, pos, len)) return QByteArray();
    if (pos >= total || buf[pos++] != 0x30 || !readAsn1Length(buf, total, pos, len)) return QByteArray();
    pos += len;
    if (pos >= total || buf[pos++] != 0x03 || !readAsn1Length(buf, total, pos, len) || len < 2) return QByteArray();
    if (buf[pos++] != 0x00) return QByteArray();
    return spki.mid(pos, len - 1);
}

} // namespace detail

inline QByteArray aesGcmEncrypt(const QByteArray &key, const QByteArray &plaintext, QByteArray &outTag, bool *ok = nullptr)
{
    if (ok) *ok = false;
    if (key.size() != 16) return QByteArray();

    const quint8 *keyBytes = reinterpret_cast<const quint8 *>(key.constData());
    quint8 h[16], j0[16], encJ0[16];
    if (!detail::gcmPrepareState(keyBytes, h, j0, encJ0)) return QByteArray();

    QByteArray out(plaintext.size(), '\0');
    const quint8 *ptPtr = reinterpret_cast<const quint8 *>(plaintext.constData());
    quint8 *ctPtr = reinterpret_cast<quint8 *>(out.data());
    if (!detail::gcmXorStream(keyBytes, j0, ptPtr, plaintext.size(), ctPtr)) return QByteArray();

    quint8 s[16];
    detail::ghashCompute(h, ctPtr, out.size(), s);
    outTag.resize(16);
    quint8 *tagPtr = reinterpret_cast<quint8 *>(outTag.data());
    for (int i = 0; i < 16; ++i) tagPtr[i] = s[i] ^ encJ0[i];

    if (ok) *ok = true;
    return out;
}

inline QByteArray aesGcmDecrypt(const QByteArray &key, const QByteArray &ciphertext, const QByteArray &tag, bool *ok = nullptr)
{
    if (ok) *ok = false;
    if (key.size() != 16 || tag.size() != 16) return QByteArray();

    const quint8 *keyBytes = reinterpret_cast<const quint8 *>(key.constData());
    quint8 h[16], j0[16], encJ0[16];
    if (!detail::gcmPrepareState(keyBytes, h, j0, encJ0)) return QByteArray();

    const quint8 *ctPtr = reinterpret_cast<const quint8 *>(ciphertext.constData());
    const quint8 *tagPtr = reinterpret_cast<const quint8 *>(tag.constData());
    quint8 s[16];
    detail::ghashCompute(h, ctPtr, ciphertext.size(), s);
    quint8 diff = 0;
    for (int i = 0; i < 16; ++i) diff |= (s[i] ^ encJ0[i] ^ tagPtr[i]);
    if (diff != 0) return QByteArray();

    QByteArray out(ciphertext.size(), '\0');
    quint8 *ptPtr = reinterpret_cast<quint8 *>(out.data());
    if (!detail::gcmXorStream(keyBytes, j0, ctPtr, ciphertext.size(), ptPtr)) return QByteArray();

    if (ok) *ok = true;
    return out;
}

inline QByteArray aesCbcEncrypt(const QByteArray &key, const QByteArray &plaintext, bool *ok = nullptr)
{
    if (ok) *ok = false;
    if (key.size() != 16) return QByteArray();

    QByteArray out(plaintext.size() + kCCBlockSizeAES128, '\0');
    size_t moved = 0;
    const quint8 dummy = 0;
    const void *inPtr = plaintext.isEmpty() ? static_cast<const void *>(&dummy)
                                            : static_cast<const void *>(plaintext.constData());
    const CCCryptorStatus st = CCCrypt(kCCEncrypt, kCCAlgorithmAES, kCCOptionPKCS7Padding,
                                       key.constData(), 16, MILINK_FIXED_IV,
                                       inPtr, static_cast<size_t>(plaintext.size()),
                                       out.data(), static_cast<size_t>(out.size()), &moved);
    if (st != kCCSuccess) return QByteArray();
    out.resize(static_cast<int>(moved));
    if (ok) *ok = true;
    return out;
}

inline QByteArray aesCbcDecrypt(const QByteArray &key, const QByteArray &ciphertext, bool *ok = nullptr)
{
    if (ok) *ok = false;
    if (key.size() != 16) return QByteArray();

    QByteArray out(ciphertext.size() + kCCBlockSizeAES128, '\0');
    size_t moved = 0;
    const quint8 dummy = 0;
    const void *inPtr = ciphertext.isEmpty() ? static_cast<const void *>(&dummy)
                                             : static_cast<const void *>(ciphertext.constData());
    const CCCryptorStatus st = CCCrypt(kCCDecrypt, kCCAlgorithmAES, kCCOptionPKCS7Padding,
                                       key.constData(), 16, MILINK_FIXED_IV,
                                       inPtr, static_cast<size_t>(ciphertext.size()),
                                       out.data(), static_cast<size_t>(out.size()), &moved);
    if (st != kCCSuccess) return QByteArray();
    out.resize(static_cast<int>(moved));
    if (ok) *ok = true;
    return out;
}

inline QByteArray rsaPkcs1Encrypt(const QByteArray &spkiB64, const QByteArray &plaintext, bool *ok = nullptr)
{
    if (ok) *ok = false;
    const QByteArray spki = QByteArray::fromBase64(spkiB64);
    const QByteArray pkcs1 = detail::spkiDerToPkcs1Der(spki);
    if (pkcs1.isEmpty()) return QByteArray();

    CFDataRef keyData = CFDataCreate(kCFAllocatorDefault,
                                     reinterpret_cast<const UInt8 *>(pkcs1.constData()),
                                     static_cast<CFIndex>(pkcs1.size()));
    if (!keyData) return QByteArray();

    const void *keys[] = { kSecAttrKeyType, kSecAttrKeyClass };
    const void *vals[] = { kSecAttrKeyTypeRSA, kSecAttrKeyClassPublic };
    CFDictionaryRef attrs = CFDictionaryCreate(kCFAllocatorDefault, keys, vals, 2,
                                               &kCFTypeDictionaryKeyCallBacks,
                                               &kCFTypeDictionaryValueCallBacks);
    CFErrorRef err = nullptr;
    SecKeyRef pubKey = SecKeyCreateWithData(keyData, attrs, &err);
    CFRelease(attrs);
    CFRelease(keyData);
    if (err) CFRelease(err);
    if (!pubKey) return QByteArray();

    const size_t keyBytes = SecKeyGetBlockSize(pubKey);
    const int maxChunk = keyBytes > 11 ? static_cast<int>(keyBytes - 11) : 117;

    QByteArray result;
    bool allGood = true;
    for (int offset = 0; offset < plaintext.size(); offset += maxChunk) {
        const QByteArray chunk = plaintext.mid(offset, maxChunk);
        CFDataRef ptRef = CFDataCreate(kCFAllocatorDefault,
                                       reinterpret_cast<const UInt8 *>(chunk.constData()),
                                       static_cast<CFIndex>(chunk.size()));
        CFErrorRef encErr = nullptr;
        CFDataRef ctRef = ptRef
            ? SecKeyCreateEncryptedData(pubKey, kSecKeyAlgorithmRSAEncryptionPKCS1, ptRef, &encErr)
            : nullptr;
        if (ptRef) CFRelease(ptRef);
        if (encErr) CFRelease(encErr);
        if (!ctRef) {
            allGood = false;
            break;
        }
        result.append(reinterpret_cast<const char *>(CFDataGetBytePtr(ctRef)),
                      static_cast<int>(CFDataGetLength(ctRef)));
        CFRelease(ctRef);
    }

    CFRelease(pubKey);
    if (!allGood) return QByteArray();
    if (ok) *ok = true;
    return result;
}

#else // OpenSSL path (Android / Linux / macOS Desktop)

inline QByteArray aesGcmEncrypt(const QByteArray &key, const QByteArray &plaintext, QByteArray &outTag, bool *ok = nullptr)
{
    if (ok) *ok = false;
    if (key.size() != 16) {
        return QByteArray();
    }
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return QByteArray();

    QByteArray out(plaintext.size() + 32, '\0');
    outTag.resize(16);
    int outLen = 0;
    int finLen = 0;
    const unsigned char dummy = 0;
    const unsigned char *inPtr = plaintext.isEmpty() ? &dummy : reinterpret_cast<const unsigned char *>(plaintext.constData());

    const bool good =
        EVP_EncryptInit_ex(ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, sizeof(MILINK_FIXED_IV), nullptr) == 1 &&
        EVP_EncryptInit_ex(ctx, nullptr, nullptr,
                           reinterpret_cast<const unsigned char *>(key.constData()),
                           MILINK_FIXED_IV) == 1 &&
        EVP_EncryptUpdate(ctx,
                          reinterpret_cast<unsigned char *>(out.data()), &outLen,
                          inPtr, plaintext.size()) == 1 &&
        EVP_EncryptFinal_ex(ctx,
                            reinterpret_cast<unsigned char *>(out.data()) + outLen, &finLen) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, outTag.data()) == 1;

    EVP_CIPHER_CTX_free(ctx);
    if (!good) {
        outTag.clear();
        return QByteArray();
    }
    out.resize(outLen + finLen);
    if (ok) *ok = true;
    return out;
}

inline QByteArray aesGcmDecrypt(const QByteArray &key, const QByteArray &ciphertext, const QByteArray &tag, bool *ok = nullptr)
{
    if (ok) *ok = false;
    if (key.size() != 16 || tag.size() != 16) {
        return QByteArray();
    }
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return QByteArray();

    QByteArray out(ciphertext.size() + 32, '\0');
    QByteArray tagCopy = tag;
    int outLen = 0;
    int finLen = 0;
    const unsigned char dummy = 0;
    const unsigned char *inPtr = ciphertext.isEmpty() ? &dummy : reinterpret_cast<const unsigned char *>(ciphertext.constData());

    const bool good =
        EVP_DecryptInit_ex(ctx, EVP_aes_128_gcm(), nullptr, nullptr, nullptr) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, sizeof(MILINK_FIXED_IV), nullptr) == 1 &&
        EVP_DecryptInit_ex(ctx, nullptr, nullptr,
                           reinterpret_cast<const unsigned char *>(key.constData()),
                           MILINK_FIXED_IV) == 1 &&
        EVP_DecryptUpdate(ctx,
                          reinterpret_cast<unsigned char *>(out.data()), &outLen,
                          inPtr, ciphertext.size()) == 1 &&
        EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tagCopy.data()) == 1 &&
        EVP_DecryptFinal_ex(ctx,
                            reinterpret_cast<unsigned char *>(out.data()) + outLen, &finLen) == 1;

    EVP_CIPHER_CTX_free(ctx);
    if (!good) {
        return QByteArray();
    }
    out.resize(outLen + finLen);
    if (ok) *ok = true;
    return out;
}

inline QByteArray aesCbcEncrypt(const QByteArray &key, const QByteArray &plaintext, bool *ok = nullptr)
{
    if (ok) *ok = false;
    if (key.size() != 16) {
        return QByteArray();
    }
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return QByteArray();

    QByteArray out(plaintext.size() + 32, '\0');
    int outLen = 0;
    int finLen = 0;
    const unsigned char dummy = 0;
    const unsigned char *inPtr = plaintext.isEmpty() ? &dummy : reinterpret_cast<const unsigned char *>(plaintext.constData());

    const bool good =
        EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr,
                           reinterpret_cast<const unsigned char *>(key.constData()),
                           MILINK_FIXED_IV) == 1 &&
        EVP_EncryptUpdate(ctx,
                          reinterpret_cast<unsigned char *>(out.data()), &outLen,
                          inPtr, plaintext.size()) == 1 &&
        EVP_EncryptFinal_ex(ctx,
                            reinterpret_cast<unsigned char *>(out.data()) + outLen, &finLen) == 1;

    EVP_CIPHER_CTX_free(ctx);
    if (!good) {
        return QByteArray();
    }
    out.resize(outLen + finLen);
    if (ok) *ok = true;
    return out;
}

inline QByteArray aesCbcDecrypt(const QByteArray &key, const QByteArray &ciphertext, bool *ok = nullptr)
{
    if (ok) *ok = false;
    if (key.size() != 16) {
        return QByteArray();
    }
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) return QByteArray();

    QByteArray out(ciphertext.size() + 32, '\0');
    int outLen = 0;
    int finLen = 0;
    const unsigned char dummy = 0;
    const unsigned char *inPtr = ciphertext.isEmpty() ? &dummy : reinterpret_cast<const unsigned char *>(ciphertext.constData());

    const bool good =
        EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), nullptr,
                           reinterpret_cast<const unsigned char *>(key.constData()),
                           MILINK_FIXED_IV) == 1 &&
        EVP_DecryptUpdate(ctx,
                          reinterpret_cast<unsigned char *>(out.data()), &outLen,
                          inPtr, ciphertext.size()) == 1 &&
        EVP_DecryptFinal_ex(ctx,
                            reinterpret_cast<unsigned char *>(out.data()) + outLen, &finLen) == 1;

    EVP_CIPHER_CTX_free(ctx);
    if (!good) {
        return QByteArray();
    }
    out.resize(outLen + finLen);
    if (ok) *ok = true;
    return out;
}

inline QByteArray rsaPkcs1Encrypt(const QByteArray &spkiB64, const QByteArray &plaintext, bool *ok = nullptr)
{
    if (ok) *ok = false;
    const QByteArray der = QByteArray::fromBase64(spkiB64);
    if (der.isEmpty()) return QByteArray();

    const unsigned char *ptr = reinterpret_cast<const unsigned char *>(der.constData());
    EVP_PKEY *pkey = d2i_PUBKEY(nullptr, &ptr, der.size());
    if (!pkey) return QByteArray();

    QByteArray result;
    bool allGood = true;
    constexpr int kBlockSize = 117; // RSA-1024 PKCS#1 v1.5 max input block

    for (int offset = 0; offset < plaintext.size(); offset += kBlockSize) {
        const QByteArray chunk = plaintext.mid(offset, kBlockSize);
        EVP_PKEY_CTX *pctx = EVP_PKEY_CTX_new(pkey, nullptr);
        if (!pctx) {
            allGood = false;
            break;
        }
        size_t outLen = 0;
        if (EVP_PKEY_encrypt_init(pctx) != 1 ||
            EVP_PKEY_CTX_set_rsa_padding(pctx, RSA_PKCS1_PADDING) != 1 ||
            EVP_PKEY_encrypt(pctx, nullptr, &outLen,
                             reinterpret_cast<const unsigned char *>(chunk.constData()),
                             static_cast<size_t>(chunk.size())) != 1) {
            EVP_PKEY_CTX_free(pctx);
            allGood = false;
            break;
        }
        QByteArray encBlock(static_cast<int>(outLen), '\0');
        if (EVP_PKEY_encrypt(pctx,
                             reinterpret_cast<unsigned char *>(encBlock.data()), &outLen,
                             reinterpret_cast<const unsigned char *>(chunk.constData()),
                             static_cast<size_t>(chunk.size())) != 1) {
            EVP_PKEY_CTX_free(pctx);
            allGood = false;
            break;
        }
        encBlock.resize(static_cast<int>(outLen));
        result.append(encBlock);
        EVP_PKEY_CTX_free(pctx);
    }

    EVP_PKEY_free(pkey);
    if (!allGood) return QByteArray();
    if (ok) *ok = true;
    return result;
}

#endif // MILINK_USE_APPLE_CRYPTO

inline QByteArray zlibCompress(const QByteArray &raw, bool *ok = nullptr)
{
    if (ok) *ok = false;
    uLongf destLen = compressBound(static_cast<uLong>(raw.size()));
    QByteArray out(static_cast<int>(destLen), '\0');
    const int rc = compress2(reinterpret_cast<Bytef *>(out.data()), &destLen,
                             reinterpret_cast<const Bytef *>(raw.constData()),
                             static_cast<uLong>(raw.size()), Z_DEFAULT_COMPRESSION);
    if (rc != Z_OK) return QByteArray();
    out.resize(static_cast<int>(destLen));
    if (ok) *ok = true;
    return out;
}

inline QByteArray zlibDecompress(const QByteArray &compressed, bool *ok = nullptr)
{
    if (ok) *ok = false;
    if (compressed.isEmpty()) {
        if (ok) *ok = true;
        return QByteArray();
    }

    z_stream strm{};
    strm.next_in = reinterpret_cast<Bytef *>(const_cast<char *>(compressed.constData()));
    strm.avail_in = static_cast<uInt>(compressed.size());
    if (inflateInit(&strm) != Z_OK) {
        return QByteArray();
    }

    QByteArray out;
    char chunk[4096];
    int rc = Z_OK;
    do {
        strm.next_out = reinterpret_cast<Bytef *>(chunk);
        strm.avail_out = sizeof(chunk);
        rc = inflate(&strm, Z_NO_FLUSH);
        if (rc != Z_OK && rc != Z_STREAM_END) {
            inflateEnd(&strm);
            return QByteArray();
        }
        const int have = static_cast<int>(sizeof(chunk) - strm.avail_out);
        if (have > 0) {
            out.append(chunk, have);
        }
    } while (rc != Z_STREAM_END);

    inflateEnd(&strm);
    if (ok) *ok = true;
    return out;
}

} // namespace Crypto

class ProtoWriter
{
public:
    void writeVarint(quint64 v)
    {
        while (v >= 0x80) {
            m_buf.append(static_cast<char>((v & 0x7FULL) | 0x80ULL));
            v >>= 7;
        }
        m_buf.append(static_cast<char>(v & 0x7FULL));
    }

    void writeFieldVarint(quint32 fieldNum, quint64 v)
    {
        writeVarint((static_cast<quint64>(fieldNum) << 3) | 0ULL);
        writeVarint(v);
    }

    void writeFieldSignedVarint(quint32 fieldNum, qint64 v)
    {
        writeFieldVarint(fieldNum, static_cast<quint64>(v));
    }

    void writeFieldBool(quint32 fieldNum, bool v)
    {
        writeFieldVarint(fieldNum, v ? 1ULL : 0ULL);
    }

    void writeFieldBytes(quint32 fieldNum, const QByteArray &data)
    {
        writeVarint((static_cast<quint64>(fieldNum) << 3) | 2ULL);
        writeVarint(static_cast<quint64>(data.size()));
        m_buf.append(data);
    }

    void writeFieldString(quint32 fieldNum, const QString &str)
    {
        writeFieldBytes(fieldNum, str.toUtf8());
    }

    const QByteArray &data() const { return m_buf; }

private:
    QByteArray m_buf;
};

struct ProtoFieldValue
{
    int wireType = 0;
    quint64 varint = 0;
    QByteArray bytes;
};

class ProtoMessage
{
public:
    static ProtoMessage parse(const QByteArray &data, bool *ok = nullptr)
    {
        ProtoMessage msg;
        if (ok) *ok = false;

        int pos = 0;
        const int len = data.size();
        const auto *raw = reinterpret_cast<const quint8 *>(data.constData());

        auto readVarint = [&](quint64 &out) -> bool {
            out = 0;
            int shift = 0;
            while (pos < len) {
                const quint8 b = raw[pos++];
                out |= (static_cast<quint64>(b & 0x7F) << shift);
                if ((b & 0x80) == 0) {
                    return true;
                }
                shift += 7;
                if (shift > 63) {
                    return false;
                }
            }
            return false;
        };

        while (pos < len) {
            quint64 tag = 0;
            if (!readVarint(tag)) {
                return ProtoMessage();
            }
            const quint32 fieldNum = static_cast<quint32>(tag >> 3);
            const int wireType = static_cast<int>(tag & 0x7ULL);

            ProtoFieldValue fv;
            fv.wireType = wireType;
            if (wireType == 0) {
                if (!readVarint(fv.varint)) {
                    return ProtoMessage();
                }
            } else if (wireType == 2) {
                quint64 byteLen = 0;
                if (!readVarint(byteLen) || byteLen > static_cast<quint64>(len - pos)) {
                    return ProtoMessage();
                }
                fv.bytes = data.mid(pos, static_cast<int>(byteLen));
                pos += static_cast<int>(byteLen);
            } else if (wireType == 1) {
                if (pos + 8 > len) {
                    return ProtoMessage();
                }
                fv.varint = qFromLittleEndian<quint64>(raw + pos);
                pos += 8;
            } else if (wireType == 5) {
                if (pos + 4 > len) {
                    return ProtoMessage();
                }
                fv.varint = qFromLittleEndian<quint32>(raw + pos);
                pos += 4;
            } else {
                return ProtoMessage();
            }
            msg.m_fields[fieldNum].append(fv);
        }

        if (ok) *ok = true;
        return msg;
    }

    bool has(quint32 fieldNum) const
    {
        return m_fields.contains(fieldNum) && !m_fields.value(fieldNum).isEmpty();
    }

    quint64 getVarint(quint32 fieldNum, quint64 def = 0) const
    {
        const auto list = m_fields.value(fieldNum);
        return list.isEmpty() ? def : list.first().varint;
    }

    qint32 getInt32(quint32 fieldNum, qint32 def = 0) const
    {
        return has(fieldNum) ? static_cast<qint32>(getVarint(fieldNum)) : def;
    }

    qint64 getInt64(quint32 fieldNum, qint64 def = 0) const
    {
        return has(fieldNum) ? static_cast<qint64>(getVarint(fieldNum)) : def;
    }

    QByteArray getBytes(quint32 fieldNum) const
    {
        const auto list = m_fields.value(fieldNum);
        return list.isEmpty() ? QByteArray() : list.first().bytes;
    }

    QString getString(quint32 fieldNum, const QString &def = QString()) const
    {
        const auto list = m_fields.value(fieldNum);
        return list.isEmpty() ? def : QString::fromUtf8(list.first().bytes);
    }

    QList<ProtoFieldValue> getAll(quint32 fieldNum) const
    {
        return m_fields.value(fieldNum);
    }

private:
    QMap<quint32, QList<ProtoFieldValue>> m_fields;
};

struct ParsedFrame
{
    quint8 version = 0;
    quint8 encodeType = 0;
    quint32 flags = 0;
    quint32 appId = 0;
    quint64 miUin = 0;
    QByteArray token;
    quint32 reserved = 0;
    quint32 seq = 0;
    QByteArray extra;
    QByteArray payload;
};

struct DownstreamMessage
{
    quint32 seq = 0;
    quint64 miUin = 0;
    qint32 mnsCode = 0;
    qint32 busiCode = 0;
    QString cmd;
    QByteArray busiBuff;
    QString mnsError;
    quint8 rawEncodeType = 0;
};

struct SignalEvent
{
    quint32 action = 0;
    quint64 roomId = 0;
    qint32 errorCode = 0;
    QString errorMsg;
    QString engine;
    QString sessionId;
    quint64 fromVuid = 0;
    qint32 groupInviteType = 0;
    QList<quint64> memberVuids;
};

inline bool tryExtractFrame(QByteArray &rxBuffer, ParsedFrame &outFrame, QString &outError)
{
    outError.clear();
    if (rxBuffer.size() < 8) {
        return false;
    }

    const uchar *raw = reinterpret_cast<const uchar *>(rxBuffer.constData());
    if (raw[0] != 'm' || raw[1] != 'l' || raw[2] != 'p' || raw[3] != 0) {
        const int magicIdx = rxBuffer.indexOf(QByteArray("mlp\0", 4));
        if (magicIdx > 0) {
            rxBuffer.remove(0, magicIdx);
            raw = reinterpret_cast<const uchar *>(rxBuffer.constData());
        } else {
            outError = QStringLiteral("Invalid MiLink magic bytes");
            rxBuffer.clear();
            return false;
        }
        if (rxBuffer.size() < 8) {
            return false;
        }
    }

    const quint32 totalLen = qFromBigEndian<quint32>(raw + 4);
    if (totalLen < 29 || totalLen > 4 * 1024 * 1024) {
        outError = QStringLiteral("Invalid MiLink frame length: %1").arg(totalLen);
        rxBuffer.clear();
        return false;
    }

    if (static_cast<quint32>(rxBuffer.size()) < totalLen) {
        return false;
    }

    const QByteArray frameBytes = rxBuffer.left(static_cast<int>(totalLen));
    rxBuffer.remove(0, static_cast<int>(totalLen));

    const uchar *p = reinterpret_cast<const uchar *>(frameBytes.constData());
    const int size = frameBytes.size();
    int off = 8;

    outFrame = ParsedFrame();
    if (off + 20 > size) {
        outError = QStringLiteral("Truncated MiLink fixed header");
        return false;
    }
    outFrame.version = p[off++];
    outFrame.encodeType = p[off++];
    outFrame.flags = qFromBigEndian<quint32>(p + off); off += 4;
    outFrame.appId = qFromBigEndian<quint32>(p + off); off += 4;
    outFrame.miUin = qFromBigEndian<quint64>(p + off); off += 8;

    const quint16 tokLen = qFromBigEndian<quint16>(p + off); off += 2;
    if (off + tokLen + 8 > size) {
        outError = QStringLiteral("Truncated MiLink token field");
        return false;
    }
    outFrame.token = frameBytes.mid(off, tokLen);
    off += tokLen;

    outFrame.reserved = qFromBigEndian<quint32>(p + off); off += 4;
    outFrame.seq = qFromBigEndian<quint32>(p + off); off += 4;

    if (outFrame.version >= 4) {
        if (off >= size) {
            outError = QStringLiteral("Truncated MiLink v4 extra length");
            return false;
        }
        const quint8 extLen = p[off++];
        if (off + extLen > size) {
            outError = QStringLiteral("Truncated MiLink v4 extra bytes");
            return false;
        }
        outFrame.extra = frameBytes.mid(off, extLen);
        off += extLen;
    }

    outFrame.payload = frameBytes.mid(off);
    return true;
}

class Builder
{
public:
    static QByteArray buildUpstreamPacket(quint32 seq,
                                          quint32 appId,
                                          const QString &cmd,
                                          const QByteArray &busiBuff,
                                          quint64 miUin = 0,
                                          const QString &tokenForTokenInfo = QString(),
                                          bool needClientInfo = true,
                                          const QString &deviceInfoStr = QStringLiteral("N/A"),
                                          bool isSystemCmd = false)
    {
        ProtoWriter up;
        up.writeFieldVarint(1, seq);
        up.writeFieldVarint(2, appId);
        up.writeFieldVarint(3, miUin);
        up.writeFieldString(4, QStringLiteral("v1-android-com.xiaomi.mico-0.1.5-p39-ip-mico-SNAPSHOT-default-zh_cn-"));
        up.writeFieldString(5, cmd);

        if (needClientInfo && !deviceInfoStr.isEmpty()) {
            up.writeFieldString(6, deviceInfoStr);
        }
        if (needClientInfo && !tokenForTokenInfo.isEmpty() && miUin > 0) {
            ProtoWriter tokenInfo;
            tokenInfo.writeFieldVarint(1, 3); // type = 3
            tokenInfo.writeFieldBytes(2, tokenForTokenInfo.toUtf8());
            up.writeFieldBytes(7, tokenInfo.data());
        }

        QByteArray actualBusi = busiBuff;
        if (!isSystemCmd && !cmd.startsWith(QStringLiteral("milink"))) {
            ProtoWriter busiControl;
            if (busiBuff.size() > 512) {
                bool compOk = false;
                const QByteArray comp = Crypto::zlibCompress(busiBuff, &compOk);
                if (compOk && !comp.isEmpty()) {
                    busiControl.writeFieldVarint(1, 1); // compFlag = 1 (zlib)
                    busiControl.writeFieldVarint(2, static_cast<quint64>(busiBuff.size()));
                    actualBusi = comp;
                }
            }
            busiControl.writeFieldBool(3, true); // isSupportComp
            up.writeFieldBytes(14, busiControl.data());
        }

        if (!actualBusi.isNull()) {
            up.writeFieldBytes(9, actualBusi);
        }
        up.writeFieldString(15, QString::number(miUin));
        return up.data();
    }

    static QByteArray buildMiLinkV4Frame(quint32 seq,
                                         quint32 appId,
                                         quint8 encodeType,
                                         quint64 miUin,
                                         const QByteArray &headerToken,
                                         const QByteArray &upstreamBytes,
                                         const QByteArray &aesKey = QByteArray(),
                                         const QByteArray &rsaPubKeyB64 = QByteArray(),
                                         quint32 flags = 0,
                                         bool *ok = nullptr)
    {
        if (ok) *ok = false;
        QByteArray extra;
        QByteArray encPayload;
        bool encOk = true;

        if (encodeType == EncodeRsaChannel) {
            encPayload = Crypto::rsaPkcs1Encrypt(rsaPubKeyB64, upstreamBytes, &encOk);
        } else if (encodeType == EncodeCbcFastLogin) {
            encPayload = Crypto::aesCbcEncrypt(aesKey, upstreamBytes, &encOk);
        } else if (encodeType == EncodeGcmMns ||
                   encodeType == EncodeGcmHandshake ||
                   encodeType == EncodeGcmType8 ||
                   encodeType == EncodeGcmDeviceLogin) {
            encPayload = Crypto::aesGcmEncrypt(aesKey, upstreamBytes, extra, &encOk);
        } else {
            encPayload = upstreamBytes;
        }

        if (!encOk) {
            return QByteArray();
        }

        const quint32 totalLen = static_cast<quint32>(37 + headerToken.size() + extra.size() + encPayload.size());
        QByteArray frame;
        frame.reserve(static_cast<int>(totalLen));
        frame.append("mlp\0", 4);

        auto appendU32 = [&](quint32 v) {
            quint32 be = qToBigEndian(v);
            frame.append(reinterpret_cast<const char *>(&be), 4);
        };
        auto appendU64 = [&](quint64 v) {
            quint64 be = qToBigEndian(v);
            frame.append(reinterpret_cast<const char *>(&be), 8);
        };
        auto appendU16 = [&](quint16 v) {
            quint16 be = qToBigEndian(v);
            frame.append(reinterpret_cast<const char *>(&be), 2);
        };

        appendU32(totalLen);
        frame.append(static_cast<char>(4)); // version = 4
        frame.append(static_cast<char>(encodeType));
        appendU32(flags);
        appendU32(appId);
        appendU64(miUin);
        appendU16(static_cast<quint16>(headerToken.size()));
        frame.append(headerToken);
        appendU32(0); // reserved
        appendU32(seq);
        frame.append(static_cast<char>(extra.size()));
        frame.append(extra);
        frame.append(encPayload);

        if (ok) *ok = true;
        return frame;
    }

    static bool decryptDownstream(const ParsedFrame &pkt,
                                  const QByteArray &aesKey,
                                  DownstreamMessage &outMsg)
    {
        outMsg = DownstreamMessage();
        outMsg.seq = pkt.seq;
        outMsg.miUin = pkt.miUin;
        outMsg.rawEncodeType = pkt.encodeType;

        QByteArray plaintext = pkt.payload;
        if (pkt.encodeType != EncodeNone && !aesKey.isEmpty()) {
            bool decOk = false;
            if (pkt.version >= 4 && pkt.extra.size() == 16) {
                plaintext = Crypto::aesGcmDecrypt(aesKey, pkt.payload, pkt.extra, &decOk);
            } else {
                plaintext = Crypto::aesCbcDecrypt(aesKey, pkt.payload, &decOk);
            }
            if (!decOk) {
                return false;
            }
        }

        bool pbOk = false;
        const ProtoMessage ds = ProtoMessage::parse(plaintext, &pbOk);
        if (!pbOk) {
            return false;
        }

        if (ds.has(1)) outMsg.seq = static_cast<quint32>(ds.getVarint(1));
        if (ds.has(2)) outMsg.miUin = ds.getVarint(2);
        outMsg.mnsCode = ds.getInt32(3, 0);
        outMsg.busiCode = ds.getInt32(4, 0);
        outMsg.cmd = ds.getString(5);
        outMsg.busiBuff = ds.getBytes(6);
        outMsg.mnsError = ds.getString(8);

        if (ds.has(10)) {
            bool bcOk = false;
            const ProtoMessage bc = ProtoMessage::parse(ds.getBytes(10), &bcOk);
            if (bcOk && bc.getInt32(1, 0) == 1 && !outMsg.busiBuff.isEmpty()) {
                bool infOk = false;
                const QByteArray inflated = Crypto::zlibDecompress(outMsg.busiBuff, &infOk);
                if (!infOk) {
                    return false;
                }
                outMsg.busiBuff = inflated;
            }
        }

        if (outMsg.cmd == QStringLiteral("milink.push") && !outMsg.busiBuff.isEmpty()) {
            bool pushOk = false;
            const ProtoMessage sp = ProtoMessage::parse(outMsg.busiBuff, &pushOk);
            if (pushOk) {
                const QString innerCmd = sp.getString(4);
                if (!innerCmd.isEmpty()) {
                    outMsg.cmd = innerCmd;
                }
                outMsg.busiBuff = sp.getBytes(5);
            }
        }
        return true;
    }

    static QByteArray buildHandshakeReq()
    {
        ProtoWriter w;
        w.writeFieldVarint(1, 1); // type = 1
        return w.data();
    }

    static QByteArray buildChannelReq(const QByteArray &privacyKey,
                                      const QString &deviceInfo = QStringLiteral("Android#MIX 2S#10"))
    {
        ProtoWriter w;
        w.writeFieldBytes(1, privacyKey);
        w.writeFieldString(2, deviceInfo);
        return w.data();
    }

    static bool parseChannelRsp(const QByteArray &data,
                                quint64 &wid,
                                QByteArray &b2,
                                QByteArray &gtkeyB2)
    {
        bool ok = false;
        const ProtoMessage m = ProtoMessage::parse(data, &ok);
        if (!ok) return false;
        wid = m.getVarint(1, 0);
        b2 = m.getBytes(2);
        gtkeyB2 = m.getBytes(3);
        return wid > 0 && !b2.isEmpty() && gtkeyB2.size() == 16;
    }

    static QByteArray buildDeviceLoginReq(quint64 mid,
                                          const QString &micoapiServiceToken,
                                          const QString &deviceModel = QStringLiteral("MIX 2S"),
                                          const QString &brand = QStringLiteral("Xiaomi"))
    {
        ProtoWriter devInfo;
        devInfo.writeFieldVarint(1, 0); // deviceType = 0 (critical: 1 triggers ret=5002)
        devInfo.writeFieldString(2, brand);
        devInfo.writeFieldString(3, brand);
        devInfo.writeFieldString(4, deviceModel);
        devInfo.writeFieldVarint(5, 1); // deviceCategory = 1
        devInfo.writeFieldString(6, QString()); // miJiaDid = ""

        ProtoWriter req;
        req.writeFieldBytes(1, devInfo.data());
        req.writeFieldVarint(2, mid);
        req.writeFieldString(3, micoapiServiceToken);
        // field 4 (lid) intentionally omitted to match working Mico flow
        req.writeFieldString(5, QStringLiteral("micoapi"));
        req.writeFieldString(6, QStringLiteral("com.xiaomi.mico"));
        return req.data();
    }

    static bool parseDeviceLoginRsp(const QByteArray &data,
                                    qint32 &retCode,
                                    quint64 &callerVuid,
                                    QString &voipServiceToken,
                                    QByteArray &securityKey,
                                    QString &errorMsg)
    {
        bool ok = false;
        const ProtoMessage m = ProtoMessage::parse(data, &ok);
        if (!ok) return false;
        retCode = m.getInt32(1, -1);
        callerVuid = m.getVarint(2, 0);
        voipServiceToken = m.getString(3);
        securityKey = m.getBytes(4);
        errorMsg = m.getString(5);
        return true;
    }

    static QByteArray buildFastLoginReq(const QString &suid = QString(),
                                        bool onoff = true,
                                        bool passportLogin = true)
    {
        ProtoWriter req;
        req.writeFieldString(1, suid);
        req.writeFieldBool(3, onoff);
        req.writeFieldBool(5, passportLogin);
        return req.data();
    }

    static bool parseFastLoginRsp(const QByteArray &data,
                                  QByteArray &realNameGtkeyB2,
                                  QByteArray &realNameB2,
                                  QString &suid)
    {
        bool ok = false;
        const ProtoMessage m = ProtoMessage::parse(data, &ok);
        if (!ok) return false;
        realNameGtkeyB2 = m.getBytes(1);
        realNameB2 = m.getBytes(2);
        suid = m.getString(3);
        return realNameGtkeyB2.size() == 16 && !realNameB2.isEmpty();
    }

    static QByteArray buildGroupInviteReq(quint64 callerMid,
                                          quint64 callerVuid,
                                          quint64 targetVuid,
                                          quint64 targetDid,
                                          const QString &targetModel,
                                          qint32 groupInviteType,
                                          const QString &sigSeq,
                                          const QString &sessionId)
    {
        ProtoWriter offlinePush;
        offlinePush.writeFieldVarint(1, callerMid);
        offlinePush.writeFieldVarint(2, targetDid);
        offlinePush.writeFieldString(3, targetModel.isEmpty() ? QStringLiteral("OH2P") : targetModel);
        offlinePush.writeFieldVarint(4, targetVuid);

        ProtoWriter xiaoaiCustom;
        xiaoaiCustom.writeFieldVarint(1, 1);
        xiaoaiCustom.writeFieldBytes(2, offlinePush.data());

        ProtoWriter extCustom;
        extCustom.writeFieldVarint(1, 1);
        extCustom.writeFieldBytes(2, xiaoaiCustom.data());

        ProtoWriter toUser;
        toUser.writeFieldVarint(1, 1); // accountType = VUID
        toUser.writeFieldString(2, QString::number(targetVuid));
        toUser.writeFieldVarint(3, targetVuid);

        ProtoWriter req;
        req.writeFieldVarint(1, ActionInvite);
        req.writeFieldString(2, sigSeq);
        req.writeFieldVarint(4, 1); // fromAccountType = VUID
        req.writeFieldString(5, QString::number(callerVuid));
        req.writeFieldVarint(6, 1); // toAccountType = VUID
        req.writeFieldString(7, QString::number(targetVuid));
        req.writeFieldVarint(8, callerVuid);
        req.writeFieldString(9, QStringLiteral("agora"));
        req.writeFieldBytes(10, toUser.data());
        req.writeFieldVarint(13, static_cast<quint64>(groupInviteType)); // 1=call, 4=direct intercom
        req.writeFieldVarint(14, 1); // mode = audio
        req.writeFieldString(16, sessionId);
        req.writeFieldBool(28, true); // isCustomMode
        req.writeFieldBytes(29, extCustom.data());
        req.writeFieldBool(30, true); // enablePushTo
        req.writeFieldVarint(31, 2); // version = 2
        return req.data();
    }

    static QByteArray buildGetAgoraTokenReq(quint64 callerVuid,
                                            qint32 type,
                                            quint64 roomId = 0)
    {
        ProtoWriter req;
        req.writeFieldVarint(1, callerVuid);
        req.writeFieldVarint(2, static_cast<quint64>(type)); // 1=sdkappid, 2=token
        if (type == 2 && roomId > 0) {
            req.writeFieldVarint(3, roomId);
        }
        return req.data();
    }

    static bool parseGetAgoraTokenRsp(const QByteArray &data,
                                      qint32 &retCode,
                                      QString &token,
                                      QString &sdkAppId)
    {
        bool ok = false;
        const ProtoMessage m = ProtoMessage::parse(data, &ok);
        if (!ok) return false;
        retCode = m.getInt32(1, -1);
        token = m.getString(2);
        sdkAppId = m.getString(3);
        return true;
    }

    static QByteArray buildGroupHeartbeatReq(quint64 callerVuid,
                                             quint64 roomId,
                                             const QString &sigSeq,
                                             const QString &sessionId)
    {
        ProtoWriter req;
        req.writeFieldVarint(1, ActionGMemberIsAlive);
        req.writeFieldString(2, sigSeq);
        req.writeFieldVarint(3, roomId);
        req.writeFieldVarint(4, 1); // VUID
        req.writeFieldString(5, QString::number(callerVuid));
        req.writeFieldVarint(8, callerVuid);
        if (!sessionId.isEmpty()) {
            req.writeFieldString(16, sessionId);
        }
        req.writeFieldVarint(31, 2);
        return req.data();
    }

    static QByteArray buildGroupCancelReq(quint64 callerVuid,
                                          quint64 targetVuid,
                                          quint64 roomId,
                                          qint32 groupInviteType,
                                          const QString &sigSeq,
                                          const QString &sessionId)
    {
        ProtoWriter toUser;
        toUser.writeFieldVarint(1, 1);
        toUser.writeFieldString(2, QString::number(targetVuid));
        toUser.writeFieldVarint(3, targetVuid);

        ProtoWriter req;
        req.writeFieldVarint(1, ActionCancel);
        req.writeFieldString(2, sigSeq);
        req.writeFieldVarint(3, roomId);
        req.writeFieldVarint(4, 1);
        req.writeFieldString(5, QString::number(callerVuid));
        if (targetVuid > 0) {
            req.writeFieldVarint(6, 1);
            req.writeFieldString(7, QString::number(targetVuid));
        }
        req.writeFieldVarint(8, callerVuid);
        if (targetVuid > 0) {
            req.writeFieldBytes(10, toUser.data());
        }
        req.writeFieldVarint(13, static_cast<quint64>(groupInviteType));
        if (!sessionId.isEmpty()) {
            req.writeFieldString(16, sessionId);
        }
        req.writeFieldVarint(31, 2);
        return req.data();
    }

    static bool parseSignalResponse(const QByteArray &data, SignalEvent &outEvent)
    {
        bool ok = false;
        const ProtoMessage m = ProtoMessage::parse(data, &ok);
        if (!ok) return false;

        outEvent = SignalEvent();
        outEvent.action = static_cast<quint32>(m.getVarint(1, 0));
        outEvent.roomId = m.getVarint(3, 0);
        outEvent.errorCode = m.getInt32(4, 0);
        outEvent.errorMsg = m.getString(5);
        outEvent.engine = m.getString(7);
        outEvent.groupInviteType = m.getInt32(12, 0);
        outEvent.sessionId = m.getString(17);

        for (const auto &fv : m.getAll(8)) {
            bool uOk = false;
            const ProtoMessage user = ProtoMessage::parse(fv.bytes, &uOk);
            if (uOk && user.has(3)) {
                outEvent.memberVuids.append(user.getVarint(3));
            }
        }
        return true;
    }

    static bool parseSignalPush(const QByteArray &data, SignalEvent &outEvent)
    {
        bool ok = false;
        const ProtoMessage m = ProtoMessage::parse(data, &ok);
        if (!ok) return false;

        outEvent = SignalEvent();
        outEvent.action = static_cast<quint32>(m.getVarint(1, 0));
        outEvent.roomId = m.getVarint(3, 0);
        outEvent.fromVuid = m.getVarint(8, 0);
        outEvent.groupInviteType = m.getInt32(14, 0);
        outEvent.engine = m.getString(16);
        outEvent.sessionId = m.getString(19);

        for (const auto &fv : m.getAll(10)) {
            bool uOk = false;
            const ProtoMessage user = ProtoMessage::parse(fv.bytes, &uOk);
            if (uOk && user.has(3)) {
                outEvent.memberVuids.append(user.getVarint(3));
            }
        }
        return true;
    }
};

} // namespace MilinkProto

#endif // MILINKPROTOUTILS_H
