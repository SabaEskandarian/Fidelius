"use strict";

var sjcl = {
    cipher: {},
    hash: {},
    keyexchange: {},
    mode: {},
    misc: {},
    codec: {},
    exception: {
        corrupt: function(a) {
            this.toString = function() {
                return "CORRUPT: " + this.message
            };
            this.message = a
        },
        invalid: function(a) {
            this.toString = function() {
                return "INVALID: " + this.message
            };
            this.message = a
        },
        bug: function(a) {
            this.toString = function() {
                return "BUG: " + this.message
            };
            this.message = a
        },
        notReady: function(a) {
            this.toString = function() {
                return "NOT READY: " + this.message
            };
            this.message = a
        }
    }
};
sjcl.cipher.aes = function(a) {
    this.M[0][0][0] || this.T();
    var b, d, c, e, f = this.M[0][4],
        g = this.M[1];
    b = a.length;
    var h = 1;
    if (4 !== b && 6 !== b && 8 !== b) throw new sjcl.exception.invalid("invalid aes key size");
    this.i = [c = a.slice(0), e = []];
    for (a = b; a < 4 * b + 28; a++) {
        d = c[a - 1];
        if (0 === a % b || 8 === b && 4 === a % b) d = f[d >>> 24] << 24 ^ f[d >> 16 & 255] << 16 ^ f[d >> 8 & 255] << 8 ^ f[d & 255], 0 === a % b && (d = d << 8 ^ d >>> 24 ^ h << 24, h = h << 1 ^ 283 * (h >> 7));
        c[a] = c[a - b] ^ d
    }
    for (b = 0; a; b++, a--) d = c[b & 3 ? a : a - 4], e[b] = 4 >= a || 4 > b ? d : g[0][f[d >>> 24]] ^ g[1][f[d >> 16 & 255]] ^ g[2][f[d >> 8 & 255]] ^ g[3][f[d &
        255]]
};
sjcl.cipher.aes.prototype = {
    encrypt: function(a) {
        return ba(this, a, 0)
    },
    decrypt: function(a) {
        return ba(this, a, 1)
    },
    M: [
        [
            [],
            [],
            [],
            [],
            []
        ],
        [
            [],
            [],
            [],
            [],
            []
        ]
    ],
    T: function() {
        var a = this.M[0],
            b = this.M[1],
            d = a[4],
            c = b[4],
            e, f, g, h = [],
            k = [],
            l, m, n, p;
        for (e = 0; 0x100 > e; e++) k[(h[e] = e << 1 ^ 283 * (e >> 7)) ^ e] = e;
        for (f = g = 0; !d[f]; f ^= l || 1, g = k[g] || 1)
            for (n = g ^ g << 1 ^ g << 2 ^ g << 3 ^ g << 4, n = n >> 8 ^ n & 255 ^ 99, d[f] = n, c[n] = f, m = h[e = h[l = h[f]]], p = 0x1010101 * m ^ 0x10001 * e ^ 0x101 * l ^ 0x1010100 * f, m = 0x101 * h[n] ^ 0x1010100 * n, e = 0; 4 > e; e++) a[e][f] = m = m << 24 ^ m >>> 8, b[e][n] = p = p << 24 ^ p >>> 8;
        for (e =
            0; 5 > e; e++) a[e] = a[e].slice(0), b[e] = b[e].slice(0)
    }
};

function ba(a, b, d) {
    if (4 !== b.length) throw new sjcl.exception.invalid("invalid aes block size");
    var c = a.i[d],
        e = b[0] ^ c[0],
        f = b[d ? 3 : 1] ^ c[1],
        g = b[2] ^ c[2];
    b = b[d ? 1 : 3] ^ c[3];
    var h, k, l, m = c.length / 4 - 2,
        n, p = 4,
        r = [0, 0, 0, 0];
    h = a.M[d];
    a = h[0];
    var t = h[1],
        I = h[2],
        H = h[3],
        x = h[4];
    for (n = 0; n < m; n++) h = a[e >>> 24] ^ t[f >> 16 & 255] ^ I[g >> 8 & 255] ^ H[b & 255] ^ c[p], k = a[f >>> 24] ^ t[g >> 16 & 255] ^ I[b >> 8 & 255] ^ H[e & 255] ^ c[p + 1], l = a[g >>> 24] ^ t[b >> 16 & 255] ^ I[e >> 8 & 255] ^ H[f & 255] ^ c[p + 2], b = a[b >>> 24] ^ t[e >> 16 & 255] ^ I[f >> 8 & 255] ^ H[g & 255] ^ c[p + 3], p += 4, e = h, f = k, g = l;
    for (n =
        0; 4 > n; n++) r[d ? 3 & -n : n] = x[e >>> 24] << 24 ^ x[f >> 16 & 255] << 16 ^ x[g >> 8 & 255] << 8 ^ x[b & 255] ^ c[p++], h = e, e = f, f = g, g = b, b = h;
    return r
}
sjcl.bitArray = {
    bitSlice: function(a, b, d) {
        a = sjcl.bitArray.ra(a.slice(b / 32), 32 - (b & 31)).slice(1);
        return void 0 === d ? a : sjcl.bitArray.clamp(a, d - b)
    },
    extract: function(a, b, d) {
        var c = Math.floor(-b - d & 31);
        return ((b + d - 1 ^ b) & -32 ? a[b / 32 | 0] << 32 - c ^ a[b / 32 + 1 | 0] >>> c : a[b / 32 | 0] >>> c) & (1 << d) - 1
    },
    concat: function(a, b) {
        if (0 === a.length || 0 === b.length) return a.concat(b);
        var d = a[a.length - 1],
            c = sjcl.bitArray.getPartial(d);
        return 32 === c ? a.concat(b) : sjcl.bitArray.ra(b, c, d | 0, a.slice(0, a.length - 1))
    },
    bitLength: function(a) {
        var b = a.length;
        return 0 === b ? 0 : 32 * (b - 1) + sjcl.bitArray.getPartial(a[b - 1])
    },
    clamp: function(a, b) {
        if (32 * a.length < b) return a;
        a = a.slice(0, Math.ceil(b / 32));
        var d = a.length;
        b = b & 31;
        0 < d && b && (a[d - 1] = sjcl.bitArray.partial(b, a[d - 1] & 2147483648 >> b - 1, 1));
        return a
    },
    partial: function(a, b, d) {
        return 32 === a ? b : (d ? b | 0 : b << 32 - a) + 0x10000000000 * a
    },
    getPartial: function(a) {
        return Math.round(a / 0x10000000000) || 32
    },
    equal: function(a, b) {
        if (sjcl.bitArray.bitLength(a) !== sjcl.bitArray.bitLength(b)) return !1;
        var d = 0,
            c;
        for (c = 0; c < a.length; c++) d |= a[c] ^ b[c];
        return 0 === d
    },
    ra: function(a, b, d, c) {
        var e;
        e = 0;
        for (void 0 === c && (c = []); 32 <= b; b -= 32) c.push(d), d = 0;
        if (0 === b) return c.concat(a);
        for (e = 0; e < a.length; e++) c.push(d | a[e] >>> b), d = a[e] << 32 - b;
        e = a.length ? a[a.length - 1] : 0;
        a = sjcl.bitArray.getPartial(e);
        c.push(sjcl.bitArray.partial(b + a & 31, 32 < b + a ? d : c.pop(), 1));
        return c
    },
    l: function(a, b) {
        return [a[0] ^ b[0], a[1] ^ b[1], a[2] ^ b[2], a[3] ^ b[3]]
    },
    byteswapM: function(a) {
        var b, d;
        for (b = 0; b < a.length; ++b) d = a[b], a[b] = d >>> 24 | d >>> 8 & 0xff00 | (d & 0xff00) << 8 | d << 24;
        return a
    }
};
sjcl.codec.utf8String = {
    fromBits: function(a) {
        var b = "",
            d = sjcl.bitArray.bitLength(a),
            c, e;
        for (c = 0; c < d / 8; c++) 0 === (c & 3) && (e = a[c / 4]), b += String.fromCharCode(e >>> 8 >>> 8 >>> 8), e <<= 8;
        return decodeURIComponent(escape(b))
    },
    toBits: function(a) {
        a = unescape(encodeURIComponent(a));
        var b = [],
            d, c = 0;
        for (d = 0; d < a.length; d++) c = c << 8 | a.charCodeAt(d), 3 === (d & 3) && (b.push(c), c = 0);
        d & 3 && b.push(sjcl.bitArray.partial(8 * (d & 3), c));
        return b
    }
};
sjcl.codec.hex = {
    fromBits: function(a) {
        var b = "",
            d;
        for (d = 0; d < a.length; d++) b += ((a[d] | 0) + 0xf00000000000).toString(16).substr(4);
        return b.substr(0, sjcl.bitArray.bitLength(a) / 4)
    },
    toBits: function(a) {
        var b, d = [],
            c;
        a = a.replace(/\s|0x/g, "");
        c = a.length;
        a = a + "00000000";
        for (b = 0; b < a.length; b += 8) d.push(parseInt(a.substr(b, 8), 16) ^ 0);
        return sjcl.bitArray.clamp(d, 4 * c)
    }
};
sjcl.codec.base32 = {
    D: "ABCDEFGHIJKLMNOPQRSTUVWXYZ234567",
    la: "0123456789ABCDEFGHIJKLMNOPQRSTUV",
    BITS: 32,
    BASE: 5,
    REMAINING: 27,
    fromBits: function(a, b, d) {
        var c = sjcl.codec.base32.BASE,
            e = sjcl.codec.base32.REMAINING,
            f = "",
            g = 0,
            h = sjcl.codec.base32.D,
            k = 0,
            l = sjcl.bitArray.bitLength(a);
        d && (h = sjcl.codec.base32.la);
        for (d = 0; f.length * c < l;) f += h.charAt((k ^ a[d] >>> g) >>> e), g < c ? (k = a[d] << c - g, g += e, d++) : (k <<= c, g -= c);
        for (; f.length & 7 && !b;) f += "=";
        return f
    },
    toBits: function(a, b) {
        a = a.replace(/\s|=/g, "").toUpperCase();
        var d = sjcl.codec.base32.BITS,
            c = sjcl.codec.base32.BASE,
            e = sjcl.codec.base32.REMAINING,
            f = [],
            g, h = 0,
            k = sjcl.codec.base32.D,
            l = 0,
            m, n = "base32";
        b && (k = sjcl.codec.base32.la, n = "base32hex");
        for (g = 0; g < a.length; g++) {
            m = k.indexOf(a.charAt(g));
            if (0 > m) {
                if (!b) try {
                    return sjcl.codec.base32hex.toBits(a)
                } catch (p) {}
                throw new sjcl.exception.invalid("this isn't " + n + "!");
            }
            h > e ? (h -= e, f.push(l ^ m >>> h), l = m << d - h) : (h += c, l ^= m << d - h)
        }
        h & 56 && f.push(sjcl.bitArray.partial(h & 56, l, 1));
        return f
    }
};
sjcl.codec.base32hex = {
    fromBits: function(a, b) {
        return sjcl.codec.base32.fromBits(a, b, 1)
    },
    toBits: function(a) {
        return sjcl.codec.base32.toBits(a, 1)
    }
};
sjcl.codec.base64 = {
    D: "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/",
    fromBits: function(a, b, d) {
        var c = "",
            e = 0,
            f = sjcl.codec.base64.D,
            g = 0,
            h = sjcl.bitArray.bitLength(a);
        d && (f = f.substr(0, 62) + "-_");
        for (d = 0; 6 * c.length < h;) c += f.charAt((g ^ a[d] >>> e) >>> 26), 6 > e ? (g = a[d] << 6 - e, e += 26, d++) : (g <<= 6, e -= 6);
        for (; c.length & 3 && !b;) c += "=";
        return c
    },
    toBits: function(a, b) {
        a = a.replace(/\s|=/g, "");
        var d = [],
            c, e = 0,
            f = sjcl.codec.base64.D,
            g = 0,
            h;
        b && (f = f.substr(0, 62) + "-_");
        for (c = 0; c < a.length; c++) {
            h = f.indexOf(a.charAt(c));
            if (0 > h) throw new sjcl.exception.invalid("this isn't base64!");
            26 < e ? (e -= 26, d.push(g ^ h >>> e), g = h << 32 - e) : (e += 6, g ^= h << 32 - e)
        }
        e & 56 && d.push(sjcl.bitArray.partial(e & 56, g, 1));
        return d
    }
};
sjcl.codec.base64url = {
    fromBits: function(a) {
        return sjcl.codec.base64.fromBits(a, 1, 1)
    },
    toBits: function(a) {
        return sjcl.codec.base64.toBits(a, 1)
    }
};
sjcl.codec.bytes = {
    fromBits: function(a) {
        var b = [],
            d = sjcl.bitArray.bitLength(a),
            c, e;
        for (c = 0; c < d / 8; c++) 0 === (c & 3) && (e = a[c / 4]), b.push(e >>> 24), e <<= 8;
        return b
    },
    toBits: function(a) {
        var b = [],
            d, c = 0;
        for (d = 0; d < a.length; d++) c = c << 8 | a[d], 3 === (d & 3) && (b.push(c), c = 0);
        d & 3 && b.push(sjcl.bitArray.partial(8 * (d & 3), c));
        return b
    }
};
sjcl.codec.z85 = {
    D: "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ.-:+=^!/*?&<>()[]{}@%$#",
    ya: [0, 68, 0, 84, 83, 82, 72, 0, 75, 76, 70, 65, 0, 63, 62, 69, 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 64, 0, 73, 66, 74, 71, 81, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 77, 0, 78, 67, 0, 0, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35, 79, 0, 80, 0, 0],
    fromBits: function(a) {
        if (!a) return null;
        if (0 !== sjcl.bitArray.bitLength(a) % 32) throw new sjcl.exception.invalid("Invalid bitArray length!");
        for (var b = "", d = sjcl.codec.z85.D, c = 0; c < a.length; ++c) {
            for (var e = a[c], f = 0, g = 0; 4 > g; ++g) f = 0x100 * f + (e >>> 8 * (4 - g - 1) & 255);
            for (e = 52200625; e;) b += d.charAt(Math.floor(f / e) % 85), e = Math.floor(e / 85)
        }
        if (b.length !== 5 * a.length) throw new sjcl.exception.invalid("Bad Z85 conversion!");
        return b
    },
    toBits: function(a) {
        if (!a) return [];
        if (0 !== a.length % 5) throw new sjcl.exception.invalid("Invalid Z85 string!");
        for (var b = [], d = 0, c = sjcl.codec.z85.ya, e = 0, f = 0, g = 0; g < a.length;)
            if (d = 85 * d + c[a[g++].charCodeAt(0) - 32], 0 === g % 5) {
                for (var h = 0x1000000; h;) e =
                    e * Math.pow(2, 8) + Math.floor(d / h) % 0x100, ++f, 4 === f && (b.push(e), f = e = 0), h = Math.floor(h / 0x100);
                d = 0
            }
        return b
    }
};
sjcl.hash.sha256 = function(a) {
    this.i[0] || this.T();
    a ? (this.c = a.c.slice(0), this.h = a.h.slice(0), this.f = a.f) : this.reset()
};
sjcl.hash.sha256.hash = function(a) {
    return (new sjcl.hash.sha256).update(a).finalize()
};
sjcl.hash.sha256.prototype = {
    blockSize: 512,
    reset: function() {
        this.c = this.A.slice(0);
        this.h = [];
        this.f = 0;
        return this
    },
    update: function(a) {
        "string" === typeof a && (a = sjcl.codec.utf8String.toBits(a));
        var b, d = this.h = sjcl.bitArray.concat(this.h, a);
        b = this.f;
        a = this.f = b + sjcl.bitArray.bitLength(a);
        if (0x1fffffffffffff < a) throw new sjcl.exception.invalid("Cannot hash more than 2^53 - 1 bits");
        if ("undefined" !== typeof Uint32Array) {
            var c = new Uint32Array(d),
                e = 0;
            for (b = 512 + b - (512 + b & 0x1ff); b <= a; b += 512) this.m(c.subarray(16 * e,
                16 * (e + 1))), e += 1;
            d.splice(0, 16 * e)
        } else
            for (b = 512 + b - (512 + b & 0x1ff); b <= a; b += 512) this.m(d.splice(0, 16));
        return this
    },
    finalize: function() {
        var a, b = this.h,
            d = this.c,
            b = sjcl.bitArray.concat(b, [sjcl.bitArray.partial(1, 1)]);
        for (a = b.length + 2; a & 15; a++) b.push(0);
        b.push(Math.floor(this.f / 0x100000000));
        for (b.push(this.f | 0); b.length;) this.m(b.splice(0, 16));
        this.reset();
        return d
    },
    A: [],
    i: [],
    T: function() {
        function a(a) {
            return 0x100000000 * (a - Math.floor(a)) | 0
        }
        for (var b = 0, d = 2, c, e; 64 > b; d++) {
            e = !0;
            for (c = 2; c * c <= d; c++)
                if (0 === d % c) {
                    e = !1;
                    break
                }
            e && (8 > b && (this.A[b] = a(Math.pow(d, .5))), this.i[b] = a(Math.pow(d, 1 / 3)), b++)
        }
    },
    m: function(a) {
        var b, d, c, e = this.c,
            f = this.i,
            g = e[0],
            h = e[1],
            k = e[2],
            l = e[3],
            m = e[4],
            n = e[5],
            p = e[6],
            r = e[7];
        for (b = 0; 64 > b; b++) 16 > b ? d = a[b] : (d = a[b + 1 & 15], c = a[b + 14 & 15], d = a[b & 15] = (d >>> 7 ^ d >>> 18 ^ d >>> 3 ^ d << 25 ^ d << 14) + (c >>> 17 ^ c >>> 19 ^ c >>> 10 ^ c << 15 ^ c << 13) + a[b & 15] + a[b + 9 & 15] | 0), d = d + r + (m >>> 6 ^ m >>> 11 ^ m >>> 25 ^ m << 26 ^ m << 21 ^ m << 7) + (p ^ m & (n ^ p)) + f[b], r = p, p = n, n = m, m = l + d | 0, l = k, k = h, h = g, g = d + (h & k ^ l & (h ^ k)) + (h >>> 2 ^ h >>> 13 ^ h >>> 22 ^ h << 30 ^ h << 19 ^ h << 10) | 0;
        e[0] = e[0] + g |
            0;
        e[1] = e[1] + h | 0;
        e[2] = e[2] + k | 0;
        e[3] = e[3] + l | 0;
        e[4] = e[4] + m | 0;
        e[5] = e[5] + n | 0;
        e[6] = e[6] + p | 0;
        e[7] = e[7] + r | 0
    }
};
sjcl.hash.sha512 = function(a) {
    this.i[0] || this.T();
    a ? (this.c = a.c.slice(0), this.h = a.h.slice(0), this.f = a.f) : this.reset()
};
sjcl.hash.sha512.hash = function(a) {
    return (new sjcl.hash.sha512).update(a).finalize()
};
sjcl.hash.sha512.prototype = {
    blockSize: 1024,
    reset: function() {
        this.c = this.A.slice(0);
        this.h = [];
        this.f = 0;
        return this
    },
    update: function(a) {
        "string" === typeof a && (a = sjcl.codec.utf8String.toBits(a));
        var b, d = this.h = sjcl.bitArray.concat(this.h, a);
        b = this.f;
        a = this.f = b + sjcl.bitArray.bitLength(a);
        if (0x1fffffffffffff < a) throw new sjcl.exception.invalid("Cannot hash more than 2^53 - 1 bits");
        if ("undefined" !== typeof Uint32Array) {
            var c = new Uint32Array(d),
                e = 0;
            for (b = 1024 + b - (1024 + b & 1023); b <= a; b += 1024) this.m(c.subarray(32 *
                e, 32 * (e + 1))), e += 1;
            d.splice(0, 32 * e)
        } else
            for (b = 1024 + b - (1024 + b & 1023); b <= a; b += 1024) this.m(d.splice(0, 32));
        return this
    },
    finalize: function() {
        var a, b = this.h,
            d = this.c,
            b = sjcl.bitArray.concat(b, [sjcl.bitArray.partial(1, 1)]);
        for (a = b.length + 4; a & 31; a++) b.push(0);
        b.push(0);
        b.push(0);
        b.push(Math.floor(this.f / 0x100000000));
        for (b.push(this.f | 0); b.length;) this.m(b.splice(0, 32));
        this.reset();
        return d
    },
    A: [],
    Fa: [12372232, 13281083, 9762859, 1914609, 15106769, 4090911, 4308331, 8266105],
    i: [],
    Ha: [2666018, 15689165, 5061423, 9034684,
        4764984, 380953, 1658779, 7176472, 197186, 7368638, 14987916, 16757986, 8096111, 1480369, 13046325, 6891156, 15813330, 5187043, 9229749, 11312229, 2818677, 10937475, 4324308, 1135541, 6741931, 11809296, 16458047, 15666916, 11046850, 698149, 229999, 945776, 13774844, 2541862, 12856045, 9810911, 11494366, 7844520, 15576806, 8533307, 15795044, 4337665, 16291729, 5553712, 15684120, 6662416, 7413802, 12308920, 13816008, 4303699, 9366425, 10176680, 13195875, 4295371, 6546291, 11712675, 15708924, 1519456, 15772530, 6568428, 6495784, 8568297, 13007125, 7492395, 2515356,
        12632583, 14740254, 7262584, 1535930, 13146278, 16321966, 1853211, 294276, 13051027, 13221564, 1051980, 4080310, 6651434, 14088940, 4675607
    ],
    T: function() {
        function a(a) {
            return 0x100000000 * (a - Math.floor(a)) | 0
        }

        function b(a) {
            return 0x10000000000 * (a - Math.floor(a)) & 255
        }
        for (var d = 0, c = 2, e, f; 80 > d; c++) {
            f = !0;
            for (e = 2; e * e <= c; e++)
                if (0 === c % e) {
                    f = !1;
                    break
                }
            f && (8 > d && (this.A[2 * d] = a(Math.pow(c, .5)), this.A[2 * d + 1] = b(Math.pow(c, .5)) << 24 | this.Fa[d]), this.i[2 * d] = a(Math.pow(c, 1 / 3)), this.i[2 * d + 1] = b(Math.pow(c, 1 / 3)) << 24 | this.Ha[d], d++)
        }
    },
    m: function(a) {
        var b,
            d, c = this.c,
            e = this.i,
            f = c[0],
            g = c[1],
            h = c[2],
            k = c[3],
            l = c[4],
            m = c[5],
            n = c[6],
            p = c[7],
            r = c[8],
            t = c[9],
            I = c[10],
            H = c[11],
            x = c[12],
            B = c[13],
            A = c[14],
            y = c[15],
            u;
        if ("undefined" !== typeof Uint32Array) {
            u = Array(160);
            for (var v = 0; 32 > v; v++) u[v] = a[v]
        } else u = a;
        var v = f,
            q = g,
            w = h,
            J = k,
            L = l,
            K = m,
            X = n,
            M = p,
            D = r,
            C = t,
            T = I,
            N = H,
            U = x,
            O = B,
            Y = A,
            P = y;
        for (a = 0; 80 > a; a++) {
            if (16 > a) b = u[2 * a], d = u[2 * a + 1];
            else {
                d = u[2 * (a - 15)];
                var z = u[2 * (a - 15) + 1];
                b = (z << 31 | d >>> 1) ^ (z << 24 | d >>> 8) ^ d >>> 7;
                var E = (d << 31 | z >>> 1) ^ (d << 24 | z >>> 8) ^ (d << 25 | z >>> 7);
                d = u[2 * (a - 2)];
                var F = u[2 * (a - 2) + 1],
                    z =
                    (F << 13 | d >>> 19) ^ (d << 3 | F >>> 29) ^ d >>> 6,
                    F = (d << 13 | F >>> 19) ^ (F << 3 | d >>> 29) ^ (d << 26 | F >>> 6),
                    Z = u[2 * (a - 7)],
                    aa = u[2 * (a - 16)],
                    Q = u[2 * (a - 16) + 1];
                d = E + u[2 * (a - 7) + 1];
                b = b + Z + (d >>> 0 < E >>> 0 ? 1 : 0);
                d += F;
                b += z + (d >>> 0 < F >>> 0 ? 1 : 0);
                d += Q;
                b += aa + (d >>> 0 < Q >>> 0 ? 1 : 0)
            }
            u[2 * a] = b |= 0;
            u[2 * a + 1] = d |= 0;
            var Z = D & T ^ ~D & U,
                ga = C & N ^ ~C & O,
                F = v & w ^ v & L ^ w & L,
                ka = q & J ^ q & K ^ J & K,
                aa = (q << 4 | v >>> 28) ^ (v << 30 | q >>> 2) ^ (v << 25 | q >>> 7),
                Q = (v << 4 | q >>> 28) ^ (q << 30 | v >>> 2) ^ (q << 25 | v >>> 7),
                la = e[2 * a],
                ha = e[2 * a + 1],
                z = P + ((D << 18 | C >>> 14) ^ (D << 14 | C >>> 18) ^ (C << 23 | D >>> 9)),
                E = Y + ((C << 18 | D >>> 14) ^ (C << 14 | D >>> 18) ^ (D <<
                    23 | C >>> 9)) + (z >>> 0 < P >>> 0 ? 1 : 0),
                z = z + ga,
                E = E + (Z + (z >>> 0 < ga >>> 0 ? 1 : 0)),
                z = z + ha,
                E = E + (la + (z >>> 0 < ha >>> 0 ? 1 : 0)),
                z = z + d | 0,
                E = E + (b + (z >>> 0 < d >>> 0 ? 1 : 0));
            d = Q + ka;
            b = aa + F + (d >>> 0 < Q >>> 0 ? 1 : 0);
            Y = U;
            P = O;
            U = T;
            O = N;
            T = D;
            N = C;
            C = M + z | 0;
            D = X + E + (C >>> 0 < M >>> 0 ? 1 : 0) | 0;
            X = L;
            M = K;
            L = w;
            K = J;
            w = v;
            J = q;
            q = z + d | 0;
            v = E + b + (q >>> 0 < z >>> 0 ? 1 : 0) | 0
        }
        g = c[1] = g + q | 0;
        c[0] = f + v + (g >>> 0 < q >>> 0 ? 1 : 0) | 0;
        k = c[3] = k + J | 0;
        c[2] = h + w + (k >>> 0 < J >>> 0 ? 1 : 0) | 0;
        m = c[5] = m + K | 0;
        c[4] = l + L + (m >>> 0 < K >>> 0 ? 1 : 0) | 0;
        p = c[7] = p + M | 0;
        c[6] = n + X + (p >>> 0 < M >>> 0 ? 1 : 0) | 0;
        t = c[9] = t + C | 0;
        c[8] = r + D + (t >>> 0 < C >>> 0 ? 1 : 0) | 0;
        H = c[11] = H + N |
            0;
        c[10] = I + T + (H >>> 0 < N >>> 0 ? 1 : 0) | 0;
        B = c[13] = B + O | 0;
        c[12] = x + U + (B >>> 0 < O >>> 0 ? 1 : 0) | 0;
        y = c[15] = y + P | 0;
        c[14] = A + Y + (y >>> 0 < P >>> 0 ? 1 : 0) | 0
    }
};
sjcl.hash.sha1 = function(a) {
    a ? (this.c = a.c.slice(0), this.h = a.h.slice(0), this.f = a.f) : this.reset()
};
sjcl.hash.sha1.hash = function(a) {
    return (new sjcl.hash.sha1).update(a).finalize()
};
sjcl.hash.sha1.prototype = {
    blockSize: 512,
    reset: function() {
        this.c = this.A.slice(0);
        this.h = [];
        this.f = 0;
        return this
    },
    update: function(a) {
        "string" === typeof a && (a = sjcl.codec.utf8String.toBits(a));
        var b, d = this.h = sjcl.bitArray.concat(this.h, a);
        b = this.f;
        a = this.f = b + sjcl.bitArray.bitLength(a);
        if (0x1fffffffffffff < a) throw new sjcl.exception.invalid("Cannot hash more than 2^53 - 1 bits");
        if ("undefined" !== typeof Uint32Array) {
            var c = new Uint32Array(d),
                e = 0;
            for (b = this.blockSize + b - (this.blockSize + b & this.blockSize - 1); b <=
                a; b += this.blockSize) this.m(c.subarray(16 * e, 16 * (e + 1))), e += 1;
            d.splice(0, 16 * e)
        } else
            for (b = this.blockSize + b - (this.blockSize + b & this.blockSize - 1); b <= a; b += this.blockSize) this.m(d.splice(0, 16));
        return this
    },
    finalize: function() {
        var a, b = this.h,
            d = this.c,
            b = sjcl.bitArray.concat(b, [sjcl.bitArray.partial(1, 1)]);
        for (a = b.length + 2; a & 15; a++) b.push(0);
        b.push(Math.floor(this.f / 0x100000000));
        for (b.push(this.f | 0); b.length;) this.m(b.splice(0, 16));
        this.reset();
        return d
    },
    A: [1732584193, 4023233417, 2562383102, 271733878, 3285377520],
    i: [1518500249, 1859775393, 2400959708, 3395469782],
    m: function(a) {
        var b, d, c, e, f, g, h = this.c,
            k;
        if ("undefined" !== typeof Uint32Array)
            for (k = Array(80), d = 0; 16 > d; d++) k[d] = a[d];
        else k = a;
        d = h[0];
        c = h[1];
        e = h[2];
        f = h[3];
        g = h[4];
        for (a = 0; 79 >= a; a++) 16 <= a && (b = k[a - 3] ^ k[a - 8] ^ k[a - 14] ^ k[a - 16], k[a] = b << 1 | b >>> 31), b = 19 >= a ? c & e | ~c & f : 39 >= a ? c ^ e ^ f : 59 >= a ? c & e | c & f | e & f : 79 >= a ? c ^ e ^ f : void 0, b = (d << 5 | d >>> 27) + b + g + k[a] + this.i[Math.floor(a / 20)] | 0, g = f, f = e, e = c << 30 | c >>> 2, c = d, d = b;
        h[0] = h[0] + d | 0;
        h[1] = h[1] + c | 0;
        h[2] = h[2] + e | 0;
        h[3] = h[3] + f | 0;
        h[4] = h[4] + g | 0
    }
};
sjcl.mode.ccm = {
    name: "ccm",
    W: [],
    listenProgress: function(a) {
        sjcl.mode.ccm.W.push(a)
    },
    unListenProgress: function(a) {
        a = sjcl.mode.ccm.W.indexOf(a); - 1 < a && sjcl.mode.ccm.W.splice(a, 1)
    },
    ha: function(a) {
        var b = sjcl.mode.ccm.W.slice(),
            d;
        for (d = 0; d < b.length; d += 1) b[d](a)
    },
    encrypt: function(a, b, d, c, e) {
        var f, g = b.slice(0),
            h = sjcl.bitArray,
            k = h.bitLength(d) / 8,
            l = h.bitLength(g) / 8;
        e = e || 64;
        c = c || [];
        if (7 > k) throw new sjcl.exception.invalid("ccm: iv must be at least 7 bytes");
        for (f = 2; 4 > f && l >>> 8 * f; f++);
        f < 15 - k && (f = 15 - k);
        d = h.clamp(d,
            8 * (15 - f));
        b = sjcl.mode.ccm.R(a, b, d, c, e, f);
        g = sjcl.mode.ccm.u(a, g, d, b, e, f);
        return h.concat(g.data, g.tag)
    },
    decrypt: function(a, b, d, c, e) {
        e = e || 64;
        c = c || [];
        var f = sjcl.bitArray,
            g = f.bitLength(d) / 8,
            h = f.bitLength(b),
            k = f.clamp(b, h - e),
            l = f.bitSlice(b, h - e),
            h = (h - e) / 8;
        if (7 > g) throw new sjcl.exception.invalid("ccm: iv must be at least 7 bytes");
        for (b = 2; 4 > b && h >>> 8 * b; b++);
        b < 15 - g && (b = 15 - g);
        d = f.clamp(d, 8 * (15 - b));
        k = sjcl.mode.ccm.u(a, k, d, l, e, b);
        a = sjcl.mode.ccm.R(a, k.data, d, c, e, b);
        if (!f.equal(k.tag, a)) throw new sjcl.exception.corrupt("ccm: tag doesn't match");
        return k.data
    },
    oa: function(a, b, d, c, e, f) {
        var g = [],
            h = sjcl.bitArray,
            k = h.l;
        c = [h.partial(8, (b.length ? 64 : 0) | c - 2 << 2 | f - 1)];
        c = h.concat(c, d);
        c[3] |= e;
        c = a.encrypt(c);
        if (b.length)
            for (d = h.bitLength(b) / 8, 65279 >= d ? g = [h.partial(16, d)] : 0xffffffff >= d && (g = h.concat([h.partial(16, 65534)], [d])), g = h.concat(g, b), b = 0; b < g.length; b += 4) c = a.encrypt(k(c, g.slice(b, b + 4).concat([0, 0, 0])));
        return c
    },
    R: function(a, b, d, c, e, f) {
        var g = sjcl.bitArray,
            h = g.l;
        e /= 8;
        if (e % 2 || 4 > e || 16 < e) throw new sjcl.exception.invalid("ccm: invalid tag length");
        if (0xffffffff < c.length || 0xffffffff < b.length) throw new sjcl.exception.bug("ccm: can't deal with 4GiB or more data");
        d = sjcl.mode.ccm.oa(a, c, d, e, g.bitLength(b) / 8, f);
        for (c = 0; c < b.length; c += 4) d = a.encrypt(h(d, b.slice(c, c + 4).concat([0, 0, 0])));
        return g.clamp(d, 8 * e)
    },
    u: function(a, b, d, c, e, f) {
        var g, h = sjcl.bitArray;
        g = h.l;
        var k = b.length,
            l = h.bitLength(b),
            m = k / 50,
            n = m;
        d = h.concat([h.partial(8, f - 1)], d).concat([0, 0, 0]).slice(0, 4);
        c = h.bitSlice(g(c, a.encrypt(d)), 0, e);
        if (!k) return {
            tag: c,
            data: []
        };
        for (g = 0; g < k; g += 4) g > m && (sjcl.mode.ccm.ha(g /
            k), m += n), d[3]++, e = a.encrypt(d), b[g] ^= e[0], b[g + 1] ^= e[1], b[g + 2] ^= e[2], b[g + 3] ^= e[3];
        return {
            tag: c,
            data: h.clamp(b, l)
        }
    }
};
void 0 === sjcl.beware && (sjcl.beware = {});
sjcl.beware["CTR mode is dangerous because it doesn't protect message integrity."] = function() {
    sjcl.mode.ctr = {
        name: "ctr",
        encrypt: function(a, b, d, c) {
            return sjcl.mode.ctr.ga(a, b, d, c)
        },
        decrypt: function(a, b, d, c) {
            return sjcl.mode.ctr.ga(a, b, d, c)
        },
        ga: function(a, b, d, c) {
            var e, f, g;
            if (c && c.length) throw new sjcl.exception.invalid("ctr can't authenticate data");
            if (128 !== sjcl.bitArray.bitLength(d)) throw new sjcl.exception.invalid("ctr iv must be 128 bits");
            if (!(c = b.length)) return [];
            d = d.slice(0);
            e = b.slice(0);
            b = sjcl.bitArray.bitLength(e);
            for (g = 0; g < c; g += 4) f = a.encrypt(d), e[g] ^= f[0], e[g + 1] ^= f[1], e[g + 2] ^= f[2], e[g + 3] ^= f[3], d[3]++;
            return sjcl.bitArray.clamp(e, b)
        }
    }
};
void 0 === sjcl.beware && (sjcl.beware = {});
sjcl.beware["CBC mode is dangerous because it doesn't protect message integrity."] = function() {
    sjcl.mode.cbc = {
        name: "cbc",
        encrypt: function(a, b, d, c) {
            if (c && c.length) throw new sjcl.exception.invalid("cbc can't authenticate data");
            if (128 !== sjcl.bitArray.bitLength(d)) throw new sjcl.exception.invalid("cbc iv must be 128 bits");
            var e = sjcl.bitArray,
                f = e.l,
                g = e.bitLength(b),
                h = 0,
                k = [];
            if (g & 7) throw new sjcl.exception.invalid("pkcs#5 padding only works for multiples of a byte");
            for (c = 0; h + 128 <= g; c += 4, h += 128) d = a.encrypt(f(d,
                b.slice(c, c + 4))), k.splice(c, 0, d[0], d[1], d[2], d[3]);
            g = 0x1010101 * (16 - (g >> 3 & 15));
            d = a.encrypt(f(d, e.concat(b, [g, g, g, g]).slice(c, c + 4)));
            k.splice(c, 0, d[0], d[1], d[2], d[3]);
            return k
        },
        decrypt: function(a, b, d, c) {
            if (c && c.length) throw new sjcl.exception.invalid("cbc can't authenticate data");
            if (128 !== sjcl.bitArray.bitLength(d)) throw new sjcl.exception.invalid("cbc iv must be 128 bits");
            if (sjcl.bitArray.bitLength(b) & 127 || !b.length) throw new sjcl.exception.corrupt("cbc ciphertext must be a positive multiple of the block size");
            var e = sjcl.bitArray,
                f = e.l,
                g, h = [];
            for (c = 0; c < b.length; c += 4) g = b.slice(c, c + 4), d = f(d, a.decrypt(g)), h.splice(c, 0, d[0], d[1], d[2], d[3]), d = g;
            g = h[c - 1] & 255;
            if (0 === g || 16 < g) throw new sjcl.exception.corrupt("pkcs#5 padding corrupt");
            d = 0x1010101 * g;
            if (!e.equal(e.bitSlice([d, d, d, d], 0, 8 * g), e.bitSlice(h, 32 * h.length - 8 * g, 32 * h.length))) throw new sjcl.exception.corrupt("pkcs#5 padding corrupt");
            return e.bitSlice(h, 0, 32 * h.length - 8 * g)
        }
    }
};
sjcl.mode.ocb2 = {
    name: "ocb2",
    encrypt: function(a, b, d, c, e, f) {
        if (128 !== sjcl.bitArray.bitLength(d)) throw new sjcl.exception.invalid("ocb iv must be 128 bits");
        var g, h = sjcl.mode.ocb2.U,
            k = sjcl.bitArray,
            l = k.l,
            m = [0, 0, 0, 0];
        d = h(a.encrypt(d));
        var n, p = [];
        c = c || [];
        e = e || 64;
        for (g = 0; g + 4 < b.length; g += 4) n = b.slice(g, g + 4), m = l(m, n), p = p.concat(l(d, a.encrypt(l(d, n)))), d = h(d);
        n = b.slice(g);
        b = k.bitLength(n);
        g = a.encrypt(l(d, [0, 0, 0, b]));
        n = k.clamp(l(n.concat([0, 0, 0]), g), b);
        m = l(m, l(n.concat([0, 0, 0]), g));
        m = a.encrypt(l(m, l(d, h(d))));
        c.length && (m = l(m, f ? c : sjcl.mode.ocb2.pmac(a, c)));
        return p.concat(k.concat(n, k.clamp(m, e)))
    },
    decrypt: function(a, b, d, c, e, f) {
        if (128 !== sjcl.bitArray.bitLength(d)) throw new sjcl.exception.invalid("ocb iv must be 128 bits");
        e = e || 64;
        var g = sjcl.mode.ocb2.U,
            h = sjcl.bitArray,
            k = h.l,
            l = [0, 0, 0, 0],
            m = g(a.encrypt(d)),
            n, p, r = sjcl.bitArray.bitLength(b) - e,
            t = [];
        c = c || [];
        for (d = 0; d + 4 < r / 32; d += 4) n = k(m, a.decrypt(k(m, b.slice(d, d + 4)))), l = k(l, n), t = t.concat(n), m = g(m);
        p = r - 32 * d;
        n = a.encrypt(k(m, [0, 0, 0, p]));
        n = k(n, h.clamp(b.slice(d), p).concat([0,
            0, 0
        ]));
        l = k(l, n);
        l = a.encrypt(k(l, k(m, g(m))));
        c.length && (l = k(l, f ? c : sjcl.mode.ocb2.pmac(a, c)));
        if (!h.equal(h.clamp(l, e), h.bitSlice(b, r))) throw new sjcl.exception.corrupt("ocb: tag doesn't match");
        return t.concat(h.clamp(n, p))
    },
    pmac: function(a, b) {
        var d, c = sjcl.mode.ocb2.U,
            e = sjcl.bitArray,
            f = e.l,
            g = [0, 0, 0, 0],
            h = a.encrypt([0, 0, 0, 0]),
            h = f(h, c(c(h)));
        for (d = 0; d + 4 < b.length; d += 4) h = c(h), g = f(g, a.encrypt(f(h, b.slice(d, d + 4))));
        d = b.slice(d);
        128 > e.bitLength(d) && (h = f(h, c(h)), d = e.concat(d, [-2147483648, 0, 0, 0]));
        g = f(g, d);
        return a.encrypt(f(c(f(h, c(h))), g))
    },
    U: function(a) {
        return [a[0] << 1 ^ a[1] >>> 31, a[1] << 1 ^ a[2] >>> 31, a[2] << 1 ^ a[3] >>> 31, a[3] << 1 ^ 135 * (a[0] >>> 31)]
    }
};
sjcl.mode.ocb2progressive = {
    createEncryptor: function(a, b, d, c, e) {
        if (128 !== sjcl.bitArray.bitLength(b)) throw new sjcl.exception.invalid("ocb iv must be 128 bits");
        var f, g = sjcl.mode.ocb2.U,
            h = sjcl.bitArray,
            k = h.l,
            l = [0, 0, 0, 0],
            m = g(a.encrypt(b)),
            n, p, r = [],
            t;
        d = d || [];
        c = c || 64;
        return {
            process: function(b) {
                if (0 == sjcl.bitArray.bitLength(b)) return [];
                var d = [];
                r = r.concat(b);
                for (f = 0; f + 4 < r.length; f += 4) n = r.slice(f, f + 4), l = k(l, n), d = d.concat(k(m, a.encrypt(k(m, n)))), m = g(m);
                r = r.slice(f);
                return d
            },
            finalize: function() {
                n = r;
                p = h.bitLength(n);
                t = a.encrypt(k(m, [0, 0, 0, p]));
                n = h.clamp(k(n.concat([0, 0, 0]), t), p);
                l = k(l, k(n.concat([0, 0, 0]), t));
                l = a.encrypt(k(l, k(m, g(m))));
                d.length && (l = k(l, e ? d : sjcl.mode.ocb2.pmac(a, d)));
                return h.concat(n, h.clamp(l, c))
            }
        }
    },
    createDecryptor: function(a, b, d, c, e) {
        if (128 !== sjcl.bitArray.bitLength(b)) throw new sjcl.exception.invalid("ocb iv must be 128 bits");
        c = c || 64;
        var f, g = sjcl.mode.ocb2.U,
            h = sjcl.bitArray,
            k = h.l,
            l = [0, 0, 0, 0],
            m = g(a.encrypt(b)),
            n, p, r = [],
            t;
        d = d || [];
        return {
            process: function(b) {
                if (0 == b.length) return [];
                var d = [];
                r = r.concat(b);
                b = sjcl.bitArray.bitLength(r);
                for (f = 0; f + 4 < (b - c) / 32; f += 4) n = k(m, a.decrypt(k(m, r.slice(f, f + 4)))), l = k(l, n), d = d.concat(n), m = g(m);
                r = r.slice(f);
                return d
            },
            finalize: function() {
                p = sjcl.bitArray.bitLength(r) - c;
                t = a.encrypt(k(m, [0, 0, 0, p]));
                n = k(t, h.clamp(r, p).concat([0, 0, 0]));
                l = k(l, n);
                l = a.encrypt(k(l, k(m, g(m))));
                d.length && (l = k(l, e ? d : sjcl.mode.ocb2.pmac(a, d)));
                if (!h.equal(h.clamp(l, c), h.bitSlice(r, p))) throw new sjcl.exception.corrupt("ocb: tag doesn't match");
                return h.clamp(n, p)
            }
        }
    }
};
sjcl.mode.gcm = {
    name: "gcm",
    encrypt: function(a, b, d, c, e) {
        var f = b.slice(0);
        b = sjcl.bitArray;
        c = c || [];
        a = sjcl.mode.gcm.u(!0, a, f, c, d, e || 128);
        return b.concat(a.data, a.tag)
    },
    decrypt: function(a, b, d, c, e) {
        var f = b.slice(0),
            g = sjcl.bitArray,
            h = g.bitLength(f);
        e = e || 128;
        c = c || [];
        e <= h ? (b = g.bitSlice(f, h - e), f = g.bitSlice(f, 0, h - e)) : (b = f, f = []);
        a = sjcl.mode.gcm.u(!1, a, f, c, d, e);
        if (!g.equal(a.tag, b)) throw new sjcl.exception.corrupt("gcm: tag doesn't match");
        return a.data
    },
    Da: function(a, b) {
        var d, c, e, f, g, h = sjcl.bitArray.l;
        e = [0, 0,
            0, 0
        ];
        f = b.slice(0);
        for (d = 0; 128 > d; d++) {
            (c = 0 !== (a[Math.floor(d / 32)] & 1 << 31 - d % 32)) && (e = h(e, f));
            g = 0 !== (f[3] & 1);
            for (c = 3; 0 < c; c--) f[c] = f[c] >>> 1 | (f[c - 1] & 1) << 31;
            f[0] >>>= 1;
            g && (f[0] ^= -0x1f000000)
        }
        return e
    },
    J: function(a, b, d) {
        var c, e = d.length;
        b = b.slice(0);
        for (c = 0; c < e; c += 4) b[0] ^= 0xffffffff & d[c], b[1] ^= 0xffffffff & d[c + 1], b[2] ^= 0xffffffff & d[c + 2], b[3] ^= 0xffffffff & d[c + 3], b = sjcl.mode.gcm.Da(b, a);
        return b
    },
    u: function(a, b, d, c, e, f) {
        var g, h, k, l, m, n, p, r, t = sjcl.bitArray;
        n = d.length;
        p = t.bitLength(d);
        r = t.bitLength(c);
        h = t.bitLength(e);
        g = b.encrypt([0, 0, 0, 0]);
        96 === h ? (e = e.slice(0), e = t.concat(e, [1])) : (e = sjcl.mode.gcm.J(g, [0, 0, 0, 0], e), e = sjcl.mode.gcm.J(g, e, [0, 0, Math.floor(h / 0x100000000), h & 0xffffffff]));
        h = sjcl.mode.gcm.J(g, [0, 0, 0, 0], c);
        m = e.slice(0);
        c = h.slice(0);
        a || (c = sjcl.mode.gcm.J(g, h, d));
        for (l = 0; l < n; l += 4) m[3]++, k = b.encrypt(m), d[l] ^= k[0], d[l + 1] ^= k[1], d[l + 2] ^= k[2], d[l + 3] ^= k[3];
        d = t.clamp(d, p);
        a && (c = sjcl.mode.gcm.J(g, h, d));
        a = [Math.floor(r / 0x100000000), r & 0xffffffff, Math.floor(p / 0x100000000), p & 0xffffffff];
        c = sjcl.mode.gcm.J(g, c, a);
        k = b.encrypt(e);
        c[0] ^= k[0];
        c[1] ^= k[1];
        c[2] ^= k[2];
        c[3] ^= k[3];
        return {
            tag: t.bitSlice(c, 0, f),
            data: d
        }
    }
};
sjcl.misc.hmac = function(a, b) {
    this.ka = b = b || sjcl.hash.sha256;
    var d = [
            [],
            []
        ],
        c, e = b.prototype.blockSize / 32;
    this.P = [new b, new b];
    a.length > e && (a = b.hash(a));
    for (c = 0; c < e; c++) d[0][c] = a[c] ^ 909522486, d[1][c] = a[c] ^ 1549556828;
    this.P[0].update(d[0]);
    this.P[1].update(d[1]);
    this.ea = new b(this.P[0])
};
sjcl.misc.hmac.prototype.encrypt = sjcl.misc.hmac.prototype.mac = function(a) {
    if (this.ta) throw new sjcl.exception.invalid("encrypt on already updated hmac called!");
    this.update(a);
    return this.digest(a)
};
sjcl.misc.hmac.prototype.reset = function() {
    this.ea = new this.ka(this.P[0]);
    this.ta = !1
};
sjcl.misc.hmac.prototype.update = function(a) {
    this.ta = !0;
    this.ea.update(a)
};
sjcl.misc.hmac.prototype.digest = function() {
    var a = this.ea.finalize(),
        a = (new this.ka(this.P[1])).update(a).finalize();
    this.reset();
    return a
};
sjcl.misc.hkdf = function(a, b, d, c, e) {
    var f, g = [];
    e = e || sjcl.hash.sha256;
    "string" === typeof c && (c = sjcl.codec.utf8String.toBits(c));
    "string" === typeof d ? d = sjcl.codec.utf8String.toBits(d) : d || (d = []);
    d = new sjcl.misc.hmac(d, e);
    d = d.mac(a);
    a = sjcl.bitArray.bitLength(d);
    a = Math.ceil(b / a);
    if (255 < a) throw new sjcl.exception.invalid("key bit length is too large for hkdf");
    d = new sjcl.misc.hmac(d, e);
    f = [];
    for (e = 1; e <= a; e++) d.update(f), d.update(c), d.update([sjcl.bitArray.partial(8, e)]), f = d.digest(), g = sjcl.bitArray.concat(g,
        f);
    return sjcl.bitArray.clamp(g, b)
};
sjcl.misc.pbkdf2 = function(a, b, d, c, e) {
    d = d || 1E4;
    if (0 > c || 0 > d) throw new sjcl.exception.invalid("invalid params to pbkdf2");
    "string" === typeof a && (a = sjcl.codec.utf8String.toBits(a));
    "string" === typeof b && (b = sjcl.codec.utf8String.toBits(b));
    e = e || sjcl.misc.hmac;
    a = new e(a);
    var f, g, h, k, l = [],
        m = sjcl.bitArray;
    for (k = 1; 32 * l.length < (c || 1); k++) {
        e = f = a.encrypt(m.concat(b, [k]));
        for (g = 1; g < d; g++)
            for (f = a.encrypt(f), h = 0; h < f.length; h++) e[h] ^= f[h];
        l = l.concat(e)
    }
    c && (l = m.clamp(l, c));
    return l
};
sjcl.misc.scrypt = function(a, b, d, c, e, f, g) {
    var h = Math.pow(2, 32) - 1,
        k = sjcl.misc.scrypt;
    d = d || 16384;
    c = c || 8;
    e = e || 1;
    if (c * e >= Math.pow(2, 30)) throw sjcl.exception.invalid("The parameters r, p must satisfy r * p < 2^30");
    if (2 > d || d & 0 != d - 1) throw sjcl.exception.invalid("The parameter N must be a power of 2.");
    if (d > h / 128 / c) throw sjcl.exception.invalid("N too big.");
    if (c > h / 128 / e) throw sjcl.exception.invalid("r too big.");
    b = sjcl.misc.pbkdf2(a, b, 1, 128 * e * c * 8, g);
    c = b.length / e;
    k.reverse(b);
    for (h = 0; h < e; h++) {
        var l = b.slice(h *
            c, (h + 1) * c);
        k.blockcopy(k.ROMix(l, d), 0, b, h * c)
    }
    k.reverse(b);
    return sjcl.misc.pbkdf2(a, b, 1, f, g)
};
sjcl.misc.scrypt.salsa20Core = function(a, b) {
    function d(a, b) {
        return a << b | a >>> 32 - b
    }
    for (var c = a.slice(0), e = b; 0 < e; e -= 2) c[4] ^= d(c[0] + c[12], 7), c[8] ^= d(c[4] + c[0], 9), c[12] ^= d(c[8] + c[4], 13), c[0] ^= d(c[12] + c[8], 18), c[9] ^= d(c[5] + c[1], 7), c[13] ^= d(c[9] + c[5], 9), c[1] ^= d(c[13] + c[9], 13), c[5] ^= d(c[1] + c[13], 18), c[14] ^= d(c[10] + c[6], 7), c[2] ^= d(c[14] + c[10], 9), c[6] ^= d(c[2] + c[14], 13), c[10] ^= d(c[6] + c[2], 18), c[3] ^= d(c[15] + c[11], 7), c[7] ^= d(c[3] + c[15], 9), c[11] ^= d(c[7] + c[3], 13), c[15] ^= d(c[11] + c[7], 18), c[1] ^= d(c[0] + c[3], 7), c[2] ^=
        d(c[1] + c[0], 9), c[3] ^= d(c[2] + c[1], 13), c[0] ^= d(c[3] + c[2], 18), c[6] ^= d(c[5] + c[4], 7), c[7] ^= d(c[6] + c[5], 9), c[4] ^= d(c[7] + c[6], 13), c[5] ^= d(c[4] + c[7], 18), c[11] ^= d(c[10] + c[9], 7), c[8] ^= d(c[11] + c[10], 9), c[9] ^= d(c[8] + c[11], 13), c[10] ^= d(c[9] + c[8], 18), c[12] ^= d(c[15] + c[14], 7), c[13] ^= d(c[12] + c[15], 9), c[14] ^= d(c[13] + c[12], 13), c[15] ^= d(c[14] + c[13], 18);
    for (e = 0; 16 > e; e++) a[e] = c[e] + a[e]
};
sjcl.misc.scrypt.blockMix = function(a) {
    for (var b = a.slice(-16), d = [], c = a.length / 16, e = sjcl.misc.scrypt, f = 0; f < c; f++) e.blockxor(a, 16 * f, b, 0, 16), e.salsa20Core(b, 8), 0 == (f & 1) ? e.blockcopy(b, 0, d, 8 * f) : e.blockcopy(b, 0, d, 8 * (f ^ 1 + c));
    return d
};
sjcl.misc.scrypt.ROMix = function(a, b) {
    for (var d = a.slice(0), c = [], e = sjcl.misc.scrypt, f = 0; f < b; f++) c.push(d.slice(0)), d = e.blockMix(d);
    for (f = 0; f < b; f++) e.blockxor(c[d[d.length - 16] & b - 1], 0, d, 0), d = e.blockMix(d);
    return d
};
sjcl.misc.scrypt.reverse = function(a) {
    for (var b in a) {
        var d = a[b] & 255,
            d = d << 8 | a[b] >>> 8 & 255,
            d = d << 8 | a[b] >>> 16 & 255,
            d = d << 8 | a[b] >>> 24 & 255;
        a[b] = d
    }
};
sjcl.misc.scrypt.blockcopy = function(a, b, d, c, e) {
    var f;
    e = e || a.length - b;
    for (f = 0; f < e; f++) d[c + f] = a[b + f] | 0
};
sjcl.misc.scrypt.blockxor = function(a, b, d, c, e) {
    var f;
    e = e || a.length - b;
    for (f = 0; f < e; f++) d[c + f] = d[c + f] ^ a[b + f] | 0
};
sjcl.prng = function(a) {
    this.s = [new sjcl.hash.sha256];
    this.K = [0];
    this.da = 0;
    this.X = {};
    this.ca = 0;
    this.ia = {};
    this.qa = this.B = this.L = this.Aa = 0;
    this.i = [0, 0, 0, 0, 0, 0, 0, 0];
    this.F = [0, 0, 0, 0];
    this.aa = void 0;
    this.ba = a;
    this.V = !1;
    this.$ = {
        progress: {},
        seeded: {}
    };
    this.O = this.za = 0;
    this.Y = 1;
    this.Z = 2;
    this.va = 0x10000;
    this.fa = [0, 48, 64, 96, 128, 192, 0x100, 384, 512, 768, 1024];
    this.wa = 3E4;
    this.ua = 80
};
sjcl.prng.prototype = {
    randomWords: function(a, b) {
        var d = [],
            c;
        c = this.isReady(b);
        var e;
        if (c === this.O) throw new sjcl.exception.notReady("generator isn't seeded");
        if (c & this.Z) {
            c = !(c & this.Y);
            e = [];
            var f = 0,
                g;
            this.qa = e[0] = (new Date).valueOf() + this.wa;
            for (g = 0; 16 > g; g++) e.push(0x100000000 * Math.random() | 0);
            for (g = 0; g < this.s.length && (e = e.concat(this.s[g].finalize()), f += this.K[g], this.K[g] = 0, c || !(this.da & 1 << g)); g++);
            this.da >= 1 << this.s.length && (this.s.push(new sjcl.hash.sha256), this.K.push(0));
            this.B -= f;
            f > this.L && (this.L =
                f);
            this.da++;
            this.i = sjcl.hash.sha256.hash(this.i.concat(e));
            this.aa = new sjcl.cipher.aes(this.i);
            for (c = 0; 4 > c && (this.F[c] = this.F[c] + 1 | 0, !this.F[c]); c++);
        }
        for (c = 0; c < a; c += 4) 0 === (c + 1) % this.va && ca(this), e = da(this), d.push(e[0], e[1], e[2], e[3]);
        ca(this);
        return d.slice(0, a)
    },
    setDefaultParanoia: function(a, b) {
        if (0 === a && "Setting paranoia=0 will ruin your security; use it only for testing" !== b) throw new sjcl.exception.invalid("Setting paranoia=0 will ruin your security; use it only for testing");
        this.ba = a
    },
    addEntropy: function(a,
        b, d) {
        d = d || "user";
        var c, e, f = (new Date).valueOf(),
            g = this.X[d],
            h = this.isReady(),
            k = 0;
        c = this.ia[d];
        void 0 === c && (c = this.ia[d] = this.Aa++);
        void 0 === g && (g = this.X[d] = 0);
        this.X[d] = (this.X[d] + 1) % this.s.length;
        switch (typeof a) {
            case "number":
                void 0 === b && (b = 1);
                this.s[g].update([c, this.ca++, 1, b, f, 1, a | 0]);
                break;
            case "object":
                d = Object.prototype.toString.call(a);
                if ("[object Uint32Array]" === d) {
                    e = [];
                    for (d = 0; d < a.length; d++) e.push(a[d]);
                    a = e
                } else
                    for ("[object Array]" !== d && (k = 1), d = 0; d < a.length && !k; d++) "number" !== typeof a[d] &&
                        (k = 1);
                if (!k) {
                    if (void 0 === b)
                        for (d = b = 0; d < a.length; d++)
                            for (e = a[d]; 0 < e;) b++, e = e >>> 1;
                    this.s[g].update([c, this.ca++, 2, b, f, a.length].concat(a))
                }
                break;
            case "string":
                void 0 === b && (b = a.length);
                this.s[g].update([c, this.ca++, 3, b, f, a.length]);
                this.s[g].update(a);
                break;
            default:
                k = 1
        }
        if (k) throw new sjcl.exception.bug("random: addEntropy only supports number, array of numbers or string");
        this.K[g] += b;
        this.B += b;
        h === this.O && (this.isReady() !== this.O && ea("seeded", Math.max(this.L, this.B)), ea("progress", this.getProgress()))
    },
    isReady: function(a) {
        a = this.fa[void 0 !== a ? a : this.ba];
        return this.L && this.L >= a ? this.K[0] > this.ua && (new Date).valueOf() > this.qa ? this.Z | this.Y : this.Y : this.B >= a ? this.Z | this.O : this.O
    },
    getProgress: function(a) {
        a = this.fa[a ? a : this.ba];
        return this.L >= a ? 1 : this.B > a ? 1 : this.B / a
    },
    startCollectors: function() {
        if (!this.V) {
            this.j = {
                loadTimeCollector: G(this, this.Ia),
                mouseCollector: G(this, this.Ja),
                keyboardCollector: G(this, this.Ga),
                accelerometerCollector: G(this, this.xa),
                touchCollector: G(this, this.La)
            };
            if (window.addEventListener) window.addEventListener("load",
                this.j.loadTimeCollector, !1), window.addEventListener("mousemove", this.j.mouseCollector, !1), window.addEventListener("keypress", this.j.keyboardCollector, !1), window.addEventListener("devicemotion", this.j.accelerometerCollector, !1), window.addEventListener("touchmove", this.j.touchCollector, !1);
            else if (document.attachEvent) document.attachEvent("onload", this.j.loadTimeCollector), document.attachEvent("onmousemove", this.j.mouseCollector), document.attachEvent("keypress", this.j.keyboardCollector);
            else throw new sjcl.exception.bug("can't attach event");
            this.V = !0
        }
    },
    stopCollectors: function() {
        this.V && (window.removeEventListener ? (window.removeEventListener("load", this.j.loadTimeCollector, !1), window.removeEventListener("mousemove", this.j.mouseCollector, !1), window.removeEventListener("keypress", this.j.keyboardCollector, !1), window.removeEventListener("devicemotion", this.j.accelerometerCollector, !1), window.removeEventListener("touchmove", this.j.touchCollector, !1)) : document.detachEvent && (document.detachEvent("onload", this.j.loadTimeCollector), document.detachEvent("onmousemove",
            this.j.mouseCollector), document.detachEvent("keypress", this.j.keyboardCollector)), this.V = !1)
    },
    addEventListener: function(a, b) {
        this.$[a][this.za++] = b
    },
    removeEventListener: function(a, b) {
        var d, c, e = this.$[a],
            f = [];
        for (c in e) e.hasOwnProperty(c) && e[c] === b && f.push(c);
        for (d = 0; d < f.length; d++) c = f[d], delete e[c]
    },
    Ga: function() {
        R(this, 1)
    },
    Ja: function(a) {
        var b, d;
        try {
            b = a.x || a.clientX || a.offsetX || 0, d = a.y || a.clientY || a.offsetY || 0
        } catch (c) {
            d = b = 0
        }
        0 != b && 0 != d && this.addEntropy([b, d], 2, "mouse");
        R(this, 0)
    },
    La: function(a) {
        a =
            a.touches[0] || a.changedTouches[0];
        this.addEntropy([a.pageX || a.clientX, a.pageY || a.clientY], 1, "touch");
        R(this, 0)
    },
    Ia: function() {
        R(this, 2)
    },
    xa: function(a) {
        a = a.accelerationIncludingGravity.x || a.accelerationIncludingGravity.y || a.accelerationIncludingGravity.z;
        if (window.orientation) {
            var b = window.orientation;
            "number" === typeof b && this.addEntropy(b, 1, "accelerometer")
        }
        a && this.addEntropy(a, 2, "accelerometer");
        R(this, 0)
    }
};

function ea(a, b) {
    var d, c = sjcl.random.$[a],
        e = [];
    for (d in c) c.hasOwnProperty(d) && e.push(c[d]);
    for (d = 0; d < e.length; d++) e[d](b)
}

function R(a, b) {
    "undefined" !== typeof window && window.performance && "function" === typeof window.performance.now ? a.addEntropy(window.performance.now(), b, "loadtime") : a.addEntropy((new Date).valueOf(), b, "loadtime")
}

function ca(a) {
    a.i = da(a).concat(da(a));
    a.aa = new sjcl.cipher.aes(a.i)
}

function da(a) {
    for (var b = 0; 4 > b && (a.F[b] = a.F[b] + 1 | 0, !a.F[b]); b++);
    return a.aa.encrypt(a.F)
}

function G(a, b) {
    return function() {
        b.apply(a, arguments)
    }
}
sjcl.random = new sjcl.prng(6);
a: try {
    var S, fa, V, ia;
    if (ia = "undefined" !== typeof module && module.exports) {
        var ja;
        try {
            ja = require("crypto")
        } catch (a) {
            ja = null
        }
        ia = fa = ja
    }
    if (ia && fa.randomBytes) S = fa.randomBytes(128), S = new Uint32Array((new Uint8Array(S)).buffer), sjcl.random.addEntropy(S, 1024, "crypto['randomBytes']");
    else if ("undefined" !== typeof window && "undefined" !== typeof Uint32Array) {
        V = new Uint32Array(32);
        if (window.crypto && window.crypto.getRandomValues) window.crypto.getRandomValues(V);
        else if (window.msCrypto && window.msCrypto.getRandomValues) window.msCrypto.getRandomValues(V);
        else break a;
        sjcl.random.addEntropy(V, 1024, "crypto['getRandomValues']")
    }
} catch (a) {
    "undefined" !== typeof window && window.console && (console.log("There was an error collecting entropy from the browser:"), console.log(a))
}
sjcl.json = {
    defaults: {
        v: 1,
        iter: 1E4,
        ks: 128,
        ts: 64,
        mode: "ccm",
        adata: "",
        cipher: "aes"
    },
    Ca: function(a, b, d, c) {
        d = d || {};
        c = c || {};
        var e = sjcl.json,
            f = e.C({
                iv: sjcl.random.randomWords(4, 0)
            }, e.defaults),
            g;
        e.C(f, d);
        d = f.adata;
        "string" === typeof f.salt && (f.salt = sjcl.codec.base64.toBits(f.salt));
        "string" === typeof f.iv && (f.iv = sjcl.codec.base64.toBits(f.iv));
        if (!sjcl.mode[f.mode] || !sjcl.cipher[f.cipher] || "string" === typeof a && 100 >= f.iter || 64 !== f.ts && 96 !== f.ts && 128 !== f.ts || 128 !== f.ks && 192 !== f.ks && 0x100 !== f.ks || 2 > f.iv.length ||
            4 < f.iv.length) throw new sjcl.exception.invalid("json encrypt: invalid parameters");
        "string" === typeof a ? (g = sjcl.misc.cachedPbkdf2(a, f), a = g.key.slice(0, f.ks / 32), f.salt = g.salt) : sjcl.ecc && a instanceof sjcl.ecc.elGamal.publicKey && (g = a.kem(), f.kemtag = g.tag, a = g.key.slice(0, f.ks / 32));
        "string" === typeof b && (b = sjcl.codec.utf8String.toBits(b));
        "string" === typeof d && (f.adata = d = sjcl.codec.utf8String.toBits(d));
        g = new sjcl.cipher[f.cipher](a);
        e.C(c, f);
        c.key = a;
        f.ct = "ccm" === f.mode && sjcl.arrayBuffer && sjcl.arrayBuffer.ccm &&
            b instanceof ArrayBuffer ? sjcl.arrayBuffer.ccm.encrypt(g, b, f.iv, d, f.ts) : sjcl.mode[f.mode].encrypt(g, b, f.iv, d, f.ts);
        return f
    },
    encrypt: function(a, b, d, c) {
        var e = sjcl.json,
            f = e.Ca.apply(e, arguments);
        return e.encode(f)
    },
    Ba: function(a, b, d, c) {
        d = d || {};
        c = c || {};
        var e = sjcl.json;
        b = e.C(e.C(e.C({}, e.defaults), b), d, !0);
        var f, g;
        f = b.adata;
        "string" === typeof b.salt && (b.salt = sjcl.codec.base64.toBits(b.salt));
        "string" === typeof b.iv && (b.iv = sjcl.codec.base64.toBits(b.iv));
        if (!sjcl.mode[b.mode] || !sjcl.cipher[b.cipher] || "string" ===
            typeof a && 100 >= b.iter || 64 !== b.ts && 96 !== b.ts && 128 !== b.ts || 128 !== b.ks && 192 !== b.ks && 0x100 !== b.ks || !b.iv || 2 > b.iv.length || 4 < b.iv.length) throw new sjcl.exception.invalid("json decrypt: invalid parameters");
        "string" === typeof a ? (g = sjcl.misc.cachedPbkdf2(a, b), a = g.key.slice(0, b.ks / 32), b.salt = g.salt) : sjcl.ecc && a instanceof sjcl.ecc.elGamal.secretKey && (a = a.unkem(sjcl.codec.base64.toBits(b.kemtag)).slice(0, b.ks / 32));
        "string" === typeof f && (f = sjcl.codec.utf8String.toBits(f));
        g = new sjcl.cipher[b.cipher](a);
        f = "ccm" ===
            b.mode && sjcl.arrayBuffer && sjcl.arrayBuffer.ccm && b.ct instanceof ArrayBuffer ? sjcl.arrayBuffer.ccm.decrypt(g, b.ct, b.iv, b.tag, f, b.ts) : sjcl.mode[b.mode].decrypt(g, b.ct, b.iv, f, b.ts);
        e.C(c, b);
        c.key = a;
        return 1 === d.raw ? f : sjcl.codec.utf8String.fromBits(f)
    },
    decrypt: function(a, b, d, c) {
        var e = sjcl.json;
        return e.Ba(a, e.decode(b), d, c)
    },
    encode: function(a) {
        var b, d = "{",
            c = "";
        for (b in a)
            if (a.hasOwnProperty(b)) {
                if (!b.match(/^[a-z0-9]+$/i)) throw new sjcl.exception.invalid("json encode: invalid property name");
                d += c + '"' +
                    b + '":';
                c = ",";
                switch (typeof a[b]) {
                    case "number":
                    case "boolean":
                        d += a[b];
                        break;
                    case "string":
                        d += '"' + escape(a[b]) + '"';
                        break;
                    case "object":
                        d += '"' + sjcl.codec.base64.fromBits(a[b], 0) + '"';
                        break;
                    default:
                        throw new sjcl.exception.bug("json encode: unsupported type");
                }
            }
        return d + "}"
    },
    decode: function(a) {
        a = a.replace(/\s/g, "");
        if (!a.match(/^\{.*\}$/)) throw new sjcl.exception.invalid("json decode: this isn't json!");
        a = a.replace(/^\{|\}$/g, "").split(/,/);
        var b = {},
            d, c;
        for (d = 0; d < a.length; d++) {
            if (!(c = a[d].match(/^\s*(?:(["']?)([a-z][a-z0-9]*)\1)\s*:\s*(?:(-?\d+)|"([a-z0-9+\/%*_.@=\-]*)"|(true|false))$/i))) throw new sjcl.exception.invalid("json decode: this isn't json!");
            null != c[3] ? b[c[2]] = parseInt(c[3], 10) : null != c[4] ? b[c[2]] = c[2].match(/^(ct|adata|salt|iv)$/) ? sjcl.codec.base64.toBits(c[4]) : unescape(c[4]) : null != c[5] && (b[c[2]] = "true" === c[5])
        }
        return b
    },
    C: function(a, b, d) {
        void 0 === a && (a = {});
        if (void 0 === b) return a;
        for (var c in b)
            if (b.hasOwnProperty(c)) {
                if (d && void 0 !== a[c] && a[c] !== b[c]) throw new sjcl.exception.invalid("required parameter overridden");
                a[c] = b[c]
            }
        return a
    },
    Na: function(a, b) {
        var d = {},
            c;
        for (c in a) a.hasOwnProperty(c) && a[c] !== b[c] && (d[c] = a[c]);
        return d
    },
    Ma: function(a,
        b) {
        var d = {},
            c;
        for (c = 0; c < b.length; c++) void 0 !== a[b[c]] && (d[b[c]] = a[b[c]]);
        return d
    }
};
sjcl.encrypt = sjcl.json.encrypt;
sjcl.decrypt = sjcl.json.decrypt;
sjcl.misc.Ka = {};
sjcl.misc.cachedPbkdf2 = function(a, b) {
    var d = sjcl.misc.Ka,
        c;
    b = b || {};
    c = b.iter || 1E3;
    d = d[a] = d[a] || {};
    c = d[c] = d[c] || {
        firstSalt: b.salt && b.salt.length ? b.salt.slice(0) : sjcl.random.randomWords(2, 0)
    };
    d = void 0 === b.salt ? c.firstSalt : b.salt;
    c[d] = c[d] || sjcl.misc.pbkdf2(a, d, b.iter);
    return {
        key: c[d].slice(0),
        salt: d.slice(0)
    }
};
sjcl.bn = function(a) {
    this.initWith(a)
};
sjcl.bn.prototype = {
    radix: 24,
    maxMul: 8,
    o: sjcl.bn,
    copy: function() {
        return new this.o(this)
    },
    initWith: function(a) {
        var b = 0,
            d;
        switch (typeof a) {
            case "object":
                this.limbs = a.limbs.slice(0);
                break;
            case "number":
                this.limbs = [a];
                this.normalize();
                break;
            case "string":
                a = a.replace(/^0x/, "");
                this.limbs = [];
                d = this.radix / 4;
                for (b = 0; b < a.length; b += d) this.limbs.push(parseInt(a.substring(Math.max(a.length - b - d, 0), a.length - b), 16));
                break;
            default:
                this.limbs = [0]
        }
        return this
    },
    equals: function(a) {
        "number" === typeof a && (a = new this.o(a));
        var b = 0,
            d;
        this.fullReduce();
        a.fullReduce();
        for (d = 0; d < this.limbs.length || d < a.limbs.length; d++) b |= this.getLimb(d) ^ a.getLimb(d);
        return 0 === b
    },
    getLimb: function(a) {
        return a >= this.limbs.length ? 0 : this.limbs[a]
    },
    greaterEquals: function(a) {
        "number" === typeof a && (a = new this.o(a));
        var b = 0,
            d = 0,
            c, e, f;
        for (c = Math.max(this.limbs.length, a.limbs.length) - 1; 0 <= c; c--) e = this.getLimb(c), f = a.getLimb(c), d |= f - e & ~b, b |= e - f & ~d;
        return (d | ~b) >>> 31
    },
    toString: function() {
        this.fullReduce();
        var a = "",
            b, d, c = this.limbs;
        for (b = 0; b < this.limbs.length; b++) {
            for (d =
                c[b].toString(16); b < this.limbs.length - 1 && 6 > d.length;) d = "0" + d;
            a = d + a
        }
        return "0x" + a
    },
    addM: function(a) {
        "object" !== typeof a && (a = new this.o(a));
        var b = this.limbs,
            d = a.limbs;
        for (a = b.length; a < d.length; a++) b[a] = 0;
        for (a = 0; a < d.length; a++) b[a] += d[a];
        return this
    },
    doubleM: function() {
        var a, b = 0,
            d, c = this.radix,
            e = this.radixMask,
            f = this.limbs;
        for (a = 0; a < f.length; a++) d = f[a], d = d + d + b, f[a] = d & e, b = d >> c;
        b && f.push(b);
        return this
    },
    halveM: function() {
        var a, b = 0,
            d, c = this.radix,
            e = this.limbs;
        for (a = e.length - 1; 0 <= a; a--) d = e[a], e[a] = d + b >>
            1, b = (d & 1) << c;
        e[e.length - 1] || e.pop();
        return this
    },
    subM: function(a) {
        "object" !== typeof a && (a = new this.o(a));
        var b = this.limbs,
            d = a.limbs;
        for (a = b.length; a < d.length; a++) b[a] = 0;
        for (a = 0; a < d.length; a++) b[a] -= d[a];
        return this
    },
    mod: function(a) {
        var b = !this.greaterEquals(new sjcl.bn(0));
        a = (new sjcl.bn(a)).normalize();
        var d = (new sjcl.bn(this)).normalize(),
            c = 0;
        for (b && (d = (new sjcl.bn(0)).subM(d).normalize()); d.greaterEquals(a); c++) a.doubleM();
        for (b && (d = a.sub(d).normalize()); 0 < c; c--) a.halveM(), d.greaterEquals(a) &&
            d.subM(a).normalize();
        return d.trim()
    },
    inverseMod: function(a) {
        var b = new sjcl.bn(1),
            d = new sjcl.bn(0),
            c = new sjcl.bn(this),
            e = new sjcl.bn(a),
            f, g = 1;
        if (!(a.limbs[0] & 1)) throw new sjcl.exception.invalid("inverseMod: p must be odd");
        do
            for (c.limbs[0] & 1 && (c.greaterEquals(e) || (f = c, c = e, e = f, f = b, b = d, d = f), c.subM(e), c.normalize(), b.greaterEquals(d) || b.addM(a), b.subM(d)), c.halveM(), b.limbs[0] & 1 && b.addM(a), b.normalize(), b.halveM(), f = g = 0; f < c.limbs.length; f++) g |= c.limbs[f]; while (g);
        if (!e.equals(1)) throw new sjcl.exception.invalid("inverseMod: p and x must be relatively prime");
        return d
    },
    add: function(a) {
        return this.copy().addM(a)
    },
    sub: function(a) {
        return this.copy().subM(a)
    },
    mul: function(a) {
        "number" === typeof a && (a = new this.o(a));
        var b, d = this.limbs,
            c = a.limbs,
            e = d.length,
            f = c.length,
            g = new this.o,
            h = g.limbs,
            k, l = this.maxMul;
        for (b = 0; b < this.limbs.length + a.limbs.length + 1; b++) h[b] = 0;
        for (b = 0; b < e; b++) {
            k = d[b];
            for (a = 0; a < f; a++) h[b + a] += k * c[a];
            --l || (l = this.maxMul, g.cnormalize())
        }
        return g.cnormalize().reduce()
    },
    square: function() {
        return this.mul(this)
    },
    power: function(a) {
        a = (new sjcl.bn(a)).normalize().trim().limbs;
        var b, d, c = new this.o(1),
            e = this;
        for (b = 0; b < a.length; b++)
            for (d = 0; d < this.radix; d++) {
                a[b] & 1 << d && (c = c.mul(e));
                if (b == a.length - 1 && 0 == a[b] >> d + 1) break;
                e = e.square()
            }
        return c
    },
    mulmod: function(a, b) {
        return this.mod(b).mul(a.mod(b)).mod(b)
    },
    powermod: function(a, b) {
        a = new sjcl.bn(a);
        b = new sjcl.bn(b);
        if (1 == (b.limbs[0] & 1)) {
            var d = this.montpowermod(a, b);
            if (0 != d) return d
        }
        for (var c, e = a.normalize().trim().limbs, f = new this.o(1), g = this, d = 0; d < e.length; d++)
            for (c = 0; c < this.radix; c++) {
                e[d] & 1 << c && (f = f.mulmod(g, b));
                if (d == e.length -
                    1 && 0 == e[d] >> c + 1) break;
                g = g.mulmod(g, b)
            }
        return f
    },
    montpowermod: function(a, b) {
        function d(a, b) {
            var d = b % a.radix;
            return (a.limbs[Math.floor(b / a.radix)] & 1 << d) >> d
        }

        function c(a, d) {
            var c, e, f = (1 << l + 1) - 1;
            c = a.mul(d);
            e = c.mul(r);
            e.limbs = e.limbs.slice(0, k.limbs.length);
            e.limbs.length == k.limbs.length && (e.limbs[k.limbs.length - 1] &= f);
            e = e.mul(b);
            e = c.add(e).normalize().trim();
            e.limbs = e.limbs.slice(k.limbs.length - 1);
            for (c = 0; c < e.limbs.length; c++) 0 < c && (e.limbs[c - 1] |= (e.limbs[c] & f) << g - l - 1), e.limbs[c] >>= l + 1;
            e.greaterEquals(b) &&
                e.subM(b);
            return e
        }
        a = (new sjcl.bn(a)).normalize().trim();
        b = new sjcl.bn(b);
        var e, f, g = this.radix,
            h = new this.o(1);
        e = this.copy();
        var k, l, m;
        m = a.bitLength();
        k = new sjcl.bn({
            limbs: b.copy().normalize().trim().limbs.map(function() {
                return 0
            })
        });
        for (l = this.radix; 0 < l; l--)
            if (1 == (b.limbs[b.limbs.length - 1] >> l & 1)) {
                k.limbs[k.limbs.length - 1] = 1 << l;
                break
            }
        if (0 == m) return this;
        m = 18 > m ? 1 : 48 > m ? 3 : 144 > m ? 4 : 768 > m ? 5 : 6;
        var n = k.copy(),
            p = b.copy();
        f = new sjcl.bn(1);
        for (var r = new sjcl.bn(0), t = k.copy(); t.greaterEquals(1);) t.halveM(), 0 ==
            (f.limbs[0] & 1) ? (f.halveM(), r.halveM()) : (f.addM(p), f.halveM(), r.halveM(), r.addM(n));
        f = f.normalize();
        r = r.normalize();
        n.doubleM();
        p = n.mulmod(n, b);
        if (!n.mul(f).sub(b.mul(r)).equals(1)) return !1;
        e = c(e, p);
        h = c(h, p);
        n = {};
        f = (1 << m - 1) - 1;
        n[1] = e.copy();
        n[2] = c(e, e);
        for (e = 1; e <= f; e++) n[2 * e + 1] = c(n[2 * e - 1], n[2]);
        for (e = a.bitLength() - 1; 0 <= e;)
            if (0 == d(a, e)) h = c(h, h), --e;
            else {
                for (p = e - m + 1; 0 == d(a, p);) p++;
                t = 0;
                for (f = p; f <= e; f++) t += d(a, f) << f - p, h = c(h, h);
                h = c(h, n[t]);
                e = p - 1
            }
        return c(h, 1)
    },
    trim: function() {
        var a = this.limbs,
            b;
        do b = a.pop();
        while (a.length && 0 === b);
        a.push(b);
        return this
    },
    reduce: function() {
        return this
    },
    fullReduce: function() {
        return this.normalize()
    },
    normalize: function() {
        var a = 0,
            b, d = this.placeVal,
            c = this.ipv,
            e, f = this.limbs,
            g = f.length,
            h = this.radixMask;
        for (b = 0; b < g || 0 !== a && -1 !== a; b++) a = (f[b] || 0) + a, e = f[b] = a & h, a = (a - e) * c; - 1 === a && (f[b - 1] -= d);
        this.trim();
        return this
    },
    cnormalize: function() {
        var a = 0,
            b, d = this.ipv,
            c, e = this.limbs,
            f = e.length,
            g = this.radixMask;
        for (b = 0; b < f - 1; b++) a = e[b] + a, c = e[b] = a & g, a = (a - c) * d;
        e[b] += a;
        return this
    },
    toBits: function(a) {
        this.fullReduce();
        a = a || this.exponent || this.bitLength();
        var b = Math.floor((a - 1) / 24),
            d = sjcl.bitArray,
            c = [d.partial((a + 7 & -8) % this.radix || this.radix, this.getLimb(b))];
        for (b--; 0 <= b; b--) c = d.concat(c, [d.partial(Math.min(this.radix, a), this.getLimb(b))]), a -= this.radix;
        return c
    },
    bitLength: function() {
        this.fullReduce();
        for (var a = this.radix * (this.limbs.length - 1), b = this.limbs[this.limbs.length - 1]; b; b >>>= 1) a++;
        return a + 7 & -8
    }
};
sjcl.bn.fromBits = function(a) {
    var b = new this,
        d = [],
        c = sjcl.bitArray,
        e = this.prototype,
        f = Math.min(this.bitLength || 0x100000000, c.bitLength(a)),
        g = f % e.radix || e.radix;
    for (d[0] = c.extract(a, 0, g); g < f; g += e.radix) d.unshift(c.extract(a, g, e.radix));
    b.limbs = d;
    return b
};
sjcl.bn.prototype.ipv = 1 / (sjcl.bn.prototype.placeVal = Math.pow(2, sjcl.bn.prototype.radix));
sjcl.bn.prototype.radixMask = (1 << sjcl.bn.prototype.radix) - 1;
sjcl.bn.pseudoMersennePrime = function(a, b) {
    function d(a) {
        this.initWith(a)
    }
    var c = d.prototype = new sjcl.bn,
        e, f;
    e = c.modOffset = Math.ceil(f = a / c.radix);
    c.exponent = a;
    c.offset = [];
    c.factor = [];
    c.minOffset = e;
    c.fullMask = 0;
    c.fullOffset = [];
    c.fullFactor = [];
    c.modulus = d.modulus = new sjcl.bn(Math.pow(2, a));
    c.fullMask = 0 | -Math.pow(2, a % c.radix);
    for (e = 0; e < b.length; e++) c.offset[e] = Math.floor(b[e][0] / c.radix - f), c.fullOffset[e] = Math.ceil(b[e][0] / c.radix - f), c.factor[e] = b[e][1] * Math.pow(.5, a - b[e][0] + c.offset[e] * c.radix), c.fullFactor[e] =
        b[e][1] * Math.pow(.5, a - b[e][0] + c.fullOffset[e] * c.radix), c.modulus.addM(new sjcl.bn(Math.pow(2, b[e][0]) * b[e][1])), c.minOffset = Math.min(c.minOffset, -c.offset[e]);
    c.o = d;
    c.modulus.cnormalize();
    c.reduce = function() {
        var a, b, d, c = this.modOffset,
            e = this.limbs,
            f = this.offset,
            p = this.offset.length,
            r = this.factor,
            t;
        for (a = this.minOffset; e.length > c;) {
            d = e.pop();
            t = e.length;
            for (b = 0; b < p; b++) e[t + f[b]] -= r[b] * d;
            a--;
            a || (e.push(0), this.cnormalize(), a = this.minOffset)
        }
        this.cnormalize();
        return this
    };
    c.sa = -1 === c.fullMask ? c.reduce :
        function() {
            var a = this.limbs,
                b = a.length - 1,
                d, c;
            this.reduce();
            if (b === this.modOffset - 1) {
                c = a[b] & this.fullMask;
                a[b] -= c;
                for (d = 0; d < this.fullOffset.length; d++) a[b + this.fullOffset[d]] -= this.fullFactor[d] * c;
                this.normalize()
            }
        };
    c.fullReduce = function() {
        var a, b;
        this.sa();
        this.addM(this.modulus);
        this.addM(this.modulus);
        this.normalize();
        this.sa();
        for (b = this.limbs.length; b < this.modOffset; b++) this.limbs[b] = 0;
        a = this.greaterEquals(this.modulus);
        for (b = 0; b < this.limbs.length; b++) this.limbs[b] -= this.modulus.limbs[b] * a;
        this.cnormalize();
        return this
    };
    c.inverse = function() {
        return this.power(this.modulus.sub(2))
    };
    d.fromBits = sjcl.bn.fromBits;
    return d
};
var W = sjcl.bn.pseudoMersennePrime;
sjcl.bn.prime = {
    p127: W(127, [
        [0, -1]
    ]),
    p25519: W(255, [
        [0, -19]
    ]),
    p192k: W(192, [
        [32, -1],
        [12, -1],
        [8, -1],
        [7, -1],
        [6, -1],
        [3, -1],
        [0, -1]
    ]),
    p224k: W(224, [
        [32, -1],
        [12, -1],
        [11, -1],
        [9, -1],
        [7, -1],
        [4, -1],
        [1, -1],
        [0, -1]
    ]),
    p256k: W(0x100, [
        [32, -1],
        [9, -1],
        [8, -1],
        [7, -1],
        [6, -1],
        [4, -1],
        [0, -1]
    ]),
    p192: W(192, [
        [0, -1],
        [64, -1]
    ]),
    p224: W(224, [
        [0, 1],
        [96, -1]
    ]),
    p256: W(0x100, [
        [0, -1],
        [96, 1],
        [192, 1],
        [224, -1]
    ]),
    p384: W(384, [
        [0, -1],
        [32, 1],
        [96, -1],
        [128, -1]
    ]),
    p521: W(521, [
        [0, -1]
    ])
};
sjcl.bn.random = function(a, b) {
    "object" !== typeof a && (a = new sjcl.bn(a));
    for (var d, c, e = a.limbs.length, f = a.limbs[e - 1] + 1, g = new sjcl.bn;;) {
        do d = sjcl.random.randomWords(e, b), 0 > d[e - 1] && (d[e - 1] += 0x100000000); while (Math.floor(d[e - 1] / f) === Math.floor(0x100000000 / f));
        d[e - 1] %= f;
        for (c = 0; c < e - 1; c++) d[c] &= a.radixMask;
        g.limbs = d;
        if (!g.greaterEquals(a)) return g
    }
};
sjcl.ecc = {};
sjcl.ecc.point = function(a, b, d) {
    void 0 === b ? this.isIdentity = !0 : (b instanceof sjcl.bn && (b = new a.field(b)), d instanceof sjcl.bn && (d = new a.field(d)), this.x = b, this.y = d, this.isIdentity = !1);
    this.curve = a
};
sjcl.ecc.point.prototype = {
    toJac: function() {
        return new sjcl.ecc.pointJac(this.curve, this.x, this.y, new this.curve.field(1))
    },
    mult: function(a) {
        return this.toJac().mult(a, this).toAffine()
    },
    mult2: function(a, b, d) {
        return this.toJac().mult2(a, this, b, d).toAffine()
    },
    multiples: function() {
        var a, b, d;
        if (void 0 === this.pa)
            for (d = this.toJac().doubl(), a = this.pa = [new sjcl.ecc.point(this.curve), this, d.toAffine()], b = 3; 16 > b; b++) d = d.add(this), a.push(d.toAffine());
        return this.pa
    },
    negate: function() {
        var a = (new this.curve.field(0)).sub(this.y).normalize().reduce();
        return new sjcl.ecc.point(this.curve, this.x, a)
    },
    isValid: function() {
        return this.y.square().equals(this.curve.b.add(this.x.mul(this.curve.a.add(this.x.square()))))
    },
    toBits: function() {
        return sjcl.bitArray.concat(this.x.toBits(), this.y.toBits())
    }
};
sjcl.ecc.pointJac = function(a, b, d, c) {
    void 0 === b ? this.isIdentity = !0 : (this.x = b, this.y = d, this.z = c, this.isIdentity = !1);
    this.curve = a
};
sjcl.ecc.pointJac.prototype = {
    add: function(a) {
        var b, d, c, e;
        if (this.curve !== a.curve) throw new sjcl.exception.invalid("sjcl['ecc']['add'](): Points must be on the same curve to add them!");
        if (this.isIdentity) return a.toJac();
        if (a.isIdentity) return this;
        b = this.z.square();
        d = a.x.mul(b).subM(this.x);
        if (d.equals(0)) return this.y.equals(a.y.mul(b.mul(this.z))) ? this.doubl() : new sjcl.ecc.pointJac(this.curve);
        b = a.y.mul(b.mul(this.z)).subM(this.y);
        c = d.square();
        a = b.square();
        e = d.square().mul(d).addM(this.x.add(this.x).mul(c));
        a = a.subM(e);
        b = this.x.mul(c).subM(a).mul(b);
        c = this.y.mul(d.square().mul(d));
        b = b.subM(c);
        d = this.z.mul(d);
        return new sjcl.ecc.pointJac(this.curve, a, b, d)
    },
    doubl: function() {
        if (this.isIdentity) return this;
        var a = this.y.square(),
            b = a.mul(this.x.mul(4)),
            d = a.square().mul(8),
            a = this.z.square(),
            c = this.curve.a.toString() == (new sjcl.bn(-3)).toString() ? this.x.sub(a).mul(3).mul(this.x.add(a)) : this.x.square().mul(3).add(a.square().mul(this.curve.a)),
            a = c.square().subM(b).subM(b),
            b = b.sub(a).mul(c).subM(d),
            d = this.y.add(this.y).mul(this.z);
        return new sjcl.ecc.pointJac(this.curve, a, b, d)
    },
    toAffine: function() {
        if (this.isIdentity || this.z.equals(0)) return new sjcl.ecc.point(this.curve);
        var a = this.z.inverse(),
            b = a.square();
        return new sjcl.ecc.point(this.curve, this.x.mul(b).fullReduce(), this.y.mul(b.mul(a)).fullReduce())
    },
    mult: function(a, b) {
        "number" === typeof a ? a = [a] : void 0 !== a.limbs && (a = a.normalize().limbs);
        var d, c, e = (new sjcl.ecc.point(this.curve)).toJac(),
            f = b.multiples();
        for (d = a.length - 1; 0 <= d; d--)
            for (c = sjcl.bn.prototype.radix - 4; 0 <= c; c -= 4) e =
                e.doubl().doubl().doubl().doubl().add(f[a[d] >> c & 15]);
        return e
    },
    mult2: function(a, b, d, c) {
        "number" === typeof a ? a = [a] : void 0 !== a.limbs && (a = a.normalize().limbs);
        "number" === typeof d ? d = [d] : void 0 !== d.limbs && (d = d.normalize().limbs);
        var e, f = (new sjcl.ecc.point(this.curve)).toJac();
        b = b.multiples();
        var g = c.multiples(),
            h, k;
        for (c = Math.max(a.length, d.length) - 1; 0 <= c; c--)
            for (h = a[c] | 0, k = d[c] | 0, e = sjcl.bn.prototype.radix - 4; 0 <= e; e -= 4) f = f.doubl().doubl().doubl().doubl().add(b[h >> e & 15]).add(g[k >> e & 15]);
        return f
    },
    negate: function() {
        return this.toAffine().negate().toJac()
    },
    isValid: function() {
        var a = this.z.square(),
            b = a.square(),
            a = b.mul(a);
        return this.y.square().equals(this.curve.b.mul(a).add(this.x.mul(this.curve.a.mul(b).add(this.x.square()))))
    }
};
sjcl.ecc.curve = function(a, b, d, c, e, f) {
    this.field = a;
    this.r = new sjcl.bn(b);
    this.a = new a(d);
    this.b = new a(c);
    this.G = new sjcl.ecc.point(this, new a(e), new a(f))
};
sjcl.ecc.curve.prototype.fromBits = function(a) {
    var b = sjcl.bitArray,
        d = this.field.prototype.exponent + 7 & -8;
    a = new sjcl.ecc.point(this, this.field.fromBits(b.bitSlice(a, 0, d)), this.field.fromBits(b.bitSlice(a, d, 2 * d)));
    if (!a.isValid()) throw new sjcl.exception.corrupt("not on the curve!");
    return a
};
sjcl.ecc.curves = {
    c192: new sjcl.ecc.curve(sjcl.bn.prime.p192, "0xffffffffffffffffffffffff99def836146bc9b1b4d22831", -3, "0x64210519e59c80e70fa7e9ab72243049feb8deecc146b9b1", "0x188da80eb03090f67cbf20eb43a18800f4ff0afd82ff1012", "0x07192b95ffc8da78631011ed6b24cdd573f977a11e794811"),
    c224: new sjcl.ecc.curve(sjcl.bn.prime.p224, "0xffffffffffffffffffffffffffff16a2e0b8f03e13dd29455c5c2a3d", -3, "0xb4050a850c04b3abf54132565044b0b7d7bfd8ba270b39432355ffb4", "0xb70e0cbd6bb4bf7f321390b94a03c1d356c21122343280d6115c1d21",
        "0xbd376388b5f723fb4c22dfe6cd4375a05a07476444d5819985007e34"),
    c256: new sjcl.ecc.curve(sjcl.bn.prime.p256, "0xffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551", -3, "0x5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b", "0x6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296", "0x4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5"),
    c384: new sjcl.ecc.curve(sjcl.bn.prime.p384, "0xffffffffffffffffffffffffffffffffffffffffffffffffc7634d81f4372ddf581a0db248b0a77aecec196accc52973", -3, "0xb3312fa7e23ee7e4988e056be3f82d19181d9c6efe8141120314088f5013875ac656398d8a2ed19d2a85c8edd3ec2aef", "0xaa87ca22be8b05378eb1c71ef320ad746e1d3b628ba79b9859f741e082542a385502f25dbf55296c3a545e3872760ab7", "0x3617de4a96262c6f5d9e98bf9292dc29f8f41dbd289a147ce9da3113b5f0b8c00a60b1ce1d7e819d7a431d7c90ea0e5f"),
    c521: new sjcl.ecc.curve(sjcl.bn.prime.p521, "0x1FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFA51868783BF2F966B7FCC0148F709A5D03BB5C9B8899C47AEBB6FB71E91386409", -3, "0x051953EB9618E1C9A1F929A21A0B68540EEA2DA725B99B315F3B8B489918EF109E156193951EC7E937B1652C0BD3BB1BF073573DF883D2C34F1EF451FD46B503F00",
        "0xC6858E06B70404E9CD9E3ECB662395B4429C648139053FB521F828AF606B4D3DBAA14B5E77EFE75928FE1DC127A2FFA8DE3348B3C1856A429BF97E7E31C2E5BD66", "0x11839296A789A3BC0045C8A5FB42C7D1BD998F54449579B446817AFBD17273E662C97EE72995EF42640C550B9013FAD0761353C7086A272C24088BE94769FD16650"),
    k192: new sjcl.ecc.curve(sjcl.bn.prime.p192k, "0xfffffffffffffffffffffffe26f2fc170f69466a74defd8d", 0, 3, "0xdb4ff10ec057e9ae26b07d0280b7f4341da5d1b1eae06c7d", "0x9b2f2f6d9c5628a7844163d015be86344082aa88d95e2f9d"),
    k224: new sjcl.ecc.curve(sjcl.bn.prime.p224k,
        "0x010000000000000000000000000001dce8d2ec6184caf0a971769fb1f7", 0, 5, "0xa1455b334df099df30fc28a169a467e9e47075a90f7e650eb6b7a45c", "0x7e089fed7fba344282cafbd6f7e319f7c0b0bd59e2ca4bdb556d61a5"),
    k256: new sjcl.ecc.curve(sjcl.bn.prime.p256k, "0xfffffffffffffffffffffffffffffffebaaedce6af48a03bbfd25e8cd0364141", 0, 7, "0x79be667ef9dcbbac55a06295ce870b07029bfcdb2dce28d959f2815b16f81798", "0x483ada7726a3c4655da4fbfc0e1108a8fd17b448a68554199c47d08ffb10d4b8")
};
sjcl.ecc.curveName = function(a) {
    for (var b in sjcl.ecc.curves)
        if (sjcl.ecc.curves.hasOwnProperty(b) && sjcl.ecc.curves[b] === a) return b;
    throw new sjcl.exception.invalid("no such curve");
};
sjcl.ecc.deserialize = function(a) {
    if (!a || !a.curve || !sjcl.ecc.curves[a.curve]) throw new sjcl.exception.invalid("invalid serialization");
    if (-1 === ["elGamal", "ecdsa"].indexOf(a.type)) throw new sjcl.exception.invalid("invalid type");
    var b = sjcl.ecc.curves[a.curve];
    if (a.secretKey) {
        if (!a.exponent) throw new sjcl.exception.invalid("invalid exponent");
        var d = new sjcl.bn(a.exponent);
        return new sjcl.ecc[a.type].secretKey(b, d)
    }
    if (!a.point) throw new sjcl.exception.invalid("invalid point");
    d = b.fromBits(sjcl.codec.hex.toBits(a.point));
    return new sjcl.ecc[a.type].publicKey(b, d)
};
sjcl.ecc.basicKey = {
    publicKey: function(a, b) {
        this.w = a;
        this.I = a.r.bitLength();
        b instanceof Array ? this.H = a.fromBits(b) : this.H = b;
        this.serialize = function() {
            var b = sjcl.ecc.curveName(a);
            return {
                type: this.getType(),
                secretKey: !1,
                point: sjcl.codec.hex.fromBits(this.H.toBits()),
                curve: b
            }
        };
        this.get = function() {
            var a = this.H.toBits(),
                b = sjcl.bitArray.bitLength(a),
                e = sjcl.bitArray.bitSlice(a, 0, b / 2),
                a = sjcl.bitArray.bitSlice(a, b / 2);
            return {
                x: e,
                y: a
            }
        }
    },
    secretKey: function(a, b) {
        this.w = a;
        this.I = a.r.bitLength();
        this.S = b;
        this.serialize =
            function() {
                var b = this.get(),
                    c = sjcl.ecc.curveName(a);
                return {
                    type: this.getType(),
                    secretKey: !0,
                    exponent: sjcl.codec.hex.fromBits(b),
                    curve: c
                }
            };
        this.get = function() {
            return this.S.toBits()
        }
    }
};
sjcl.ecc.basicKey.generateKeys = function(a) {
    return function(b, d, c) {
        b = b || 0x100;
        if ("number" === typeof b && (b = sjcl.ecc.curves["c" + b], void 0 === b)) throw new sjcl.exception.invalid("no such curve");
        c = c || sjcl.bn.random(b.r, d);
        d = b.G.mult(c);
        return {
            pub: new sjcl.ecc[a].publicKey(b, d),
            sec: new sjcl.ecc[a].secretKey(b, c)
        }
    }
};
sjcl.ecc.elGamal = {
    generateKeys: sjcl.ecc.basicKey.generateKeys("elGamal"),
    publicKey: function(a, b) {
        sjcl.ecc.basicKey.publicKey.apply(this, arguments)
    },
    secretKey: function(a, b) {
        sjcl.ecc.basicKey.secretKey.apply(this, arguments)
    }
};
sjcl.ecc.elGamal.publicKey.prototype = {
    kem: function(a) {
        a = sjcl.bn.random(this.w.r, a);
        var b = this.w.G.mult(a).toBits();
        return {
            key: sjcl.hash.sha256.hash(this.H.mult(a).toBits()),
            tag: b
        }
    },
    getType: function() {
        return "elGamal"
    }
};
sjcl.ecc.elGamal.secretKey.prototype = {
    unkem: function(a) {
        return sjcl.hash.sha256.hash(this.w.fromBits(a).mult(this.S).toBits())
    },
    dh: function(a) {
        return sjcl.hash.sha256.hash(a.H.mult(this.S).toBits())
    },
    dhJavaEc: function(a) {
        return a.H.mult(this.S).x.toBits()
    },
    getType: function() {
        return "elGamal"
    }
};
sjcl.ecc.ecdsa = {
    generateKeys: sjcl.ecc.basicKey.generateKeys("ecdsa")
};
sjcl.ecc.ecdsa.publicKey = function(a, b) {
    sjcl.ecc.basicKey.publicKey.apply(this, arguments)
};
sjcl.ecc.ecdsa.publicKey.prototype = {
    verify: function(a, b, d) {
        sjcl.bitArray.bitLength(a) > this.I && (a = sjcl.bitArray.clamp(a, this.I));
        var c = sjcl.bitArray,
            e = this.w.r,
            f = this.I,
            g = sjcl.bn.fromBits(c.bitSlice(b, 0, f)),
            c = sjcl.bn.fromBits(c.bitSlice(b, f, 2 * f)),
            h = d ? c : c.inverseMod(e),
            f = sjcl.bn.fromBits(a).mul(h).mod(e),
            h = g.mul(h).mod(e),
            f = this.w.G.mult2(f, h, this.H).x;
        if (g.equals(0) || c.equals(0) || g.greaterEquals(e) || c.greaterEquals(e) || !f.equals(g)) {
            if (void 0 === d) return this.verify(a, b, !0);
            throw new sjcl.exception.corrupt("signature didn't check out");
        }
        return !0
    },
    getType: function() {
        return "ecdsa"
    }
};
sjcl.ecc.ecdsa.secretKey = function(a, b) {
    sjcl.ecc.basicKey.secretKey.apply(this, arguments)
};
sjcl.ecc.ecdsa.secretKey.prototype = {
    sign: function(a, b, d, c) {
        sjcl.bitArray.bitLength(a) > this.I && (a = sjcl.bitArray.clamp(a, this.I));
        var e = this.w.r,
            f = e.bitLength();
        c = c || sjcl.bn.random(e.sub(1), b).add(1);
        b = this.w.G.mult(c).x.mod(e);
        a = sjcl.bn.fromBits(a).add(b.mul(this.S));
        d = d ? a.inverseMod(e).mul(c).mod(e) : a.mul(c.inverseMod(e)).mod(e);
        return sjcl.bitArray.concat(b.toBits(f), d.toBits(f))
    },
    getType: function() {
        return "ecdsa"
    }
};
sjcl.keyexchange.srp = {
    makeVerifier: function(a, b, d, c) {
        a = sjcl.keyexchange.srp.makeX(a, b, d);
        a = sjcl.bn.fromBits(a);
        return c.g.powermod(a, c.N)
    },
    makeX: function(a, b, d) {
        a = sjcl.hash.sha1.hash(a + ":" + b);
        return sjcl.hash.sha1.hash(sjcl.bitArray.concat(d, a))
    },
    knownGroup: function(a) {
        "string" !== typeof a && (a = a.toString());
        sjcl.keyexchange.srp.ja || sjcl.keyexchange.srp.Ea();
        return sjcl.keyexchange.srp.na[a]
    },
    ja: !1,
    Ea: function() {
        var a, b;
        for (a = 0; a < sjcl.keyexchange.srp.ma.length; a++) b = sjcl.keyexchange.srp.ma[a].toString(),
            b = sjcl.keyexchange.srp.na[b], b.N = new sjcl.bn(b.N), b.g = new sjcl.bn(b.g);
        sjcl.keyexchange.srp.ja = !0
    },
    ma: [1024, 1536, 2048, 3072, 0x1000, 6144, 8192],
    na: {
        1024: {
            N: "EEAF0AB9ADB38DD69C33F80AFA8FC5E86072618775FF3C0B9EA2314C9C256576D674DF7496EA81D3383B4813D692C6E0E0D5D8E250B98BE48E495C1D6089DAD15DC7D7B46154D6B6CE8EF4AD69B15D4982559B297BCF1885C529F566660E57EC68EDBC3C05726CC02FD4CBF4976EAA9AFD5138FE8376435B9FC61D2FC0EB06E3",
            g: 2
        },
        1536: {
            N: "9DEF3CAFB939277AB1F12A8617A47BBBDBA51DF499AC4C80BEEEA9614B19CC4D5F4F5F556E27CBDE51C6A94BE4607A291558903BA0D0F84380B655BB9A22E8DCDF028A7CEC67F0D08134B1C8B97989149B609E0BE3BAB63D47548381DBC5B1FC764E3F4B53DD9DA1158BFD3E2B9C8CF56EDF019539349627DB2FD53D24B7C48665772E437D6C7F8CE442734AF7CCB7AE837C264AE3A9BEB87F8A2FE9B8B5292E5A021FFF5E91479E8CE7A28C2442C6F315180F93499A234DCF76E3FED135F9BB",
            g: 2
        },
        2048: {
            N: "AC6BDB41324A9A9BF166DE5E1389582FAF72B6651987EE07FC3192943DB56050A37329CBB4A099ED8193E0757767A13DD52312AB4B03310DCD7F48A9DA04FD50E8083969EDB767B0CF6095179A163AB3661A05FBD5FAAAE82918A9962F0B93B855F97993EC975EEAA80D740ADBF4FF747359D041D5C33EA71D281E446B14773BCA97B43A23FB801676BD207A436C6481F1D2B9078717461A5B9D32E688F87748544523B524B0D57D5EA77A2775D2ECFA032CFBDBF52FB3786160279004E57AE6AF874E7303CE53299CCC041C7BC308D82A5698F3A8D0C38271AE35F8E9DBFBB694B5C803D89F7AE435DE236D525F54759B65E372FCD68EF20FA7111F9E4AFF73",
            g: 2
        },
        3072: {
            N: "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AAAC42DAD33170D04507A33A85521ABDF1CBA64ECFB850458DBEF0A8AEA71575D060C7DB3970F85A6E1E4C7ABF5AE8CDB0933D71E8C94E04A25619DCEE3D2261AD2EE6BF12FFA06D98A0864D87602733EC86A64521F2B18177B200CBBE117577A615D6C770988C0BAD946E208E24FA074E5AB3143DB5BFCE0FD108E4B82D120A93AD2CAFFFFFFFFFFFFFFFF",
            g: 5
        },
        0x1000: {
            N: "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AAAC42DAD33170D04507A33A85521ABDF1CBA64ECFB850458DBEF0A8AEA71575D060C7DB3970F85A6E1E4C7ABF5AE8CDB0933D71E8C94E04A25619DCEE3D2261AD2EE6BF12FFA06D98A0864D87602733EC86A64521F2B18177B200CBBE117577A615D6C770988C0BAD946E208E24FA074E5AB3143DB5BFCE0FD108E4B82D120A92108011A723C12A787E6D788719A10BDBA5B2699C327186AF4E23C1A946834B6150BDA2583E9CA2AD44CE8DBBBC2DB04DE8EF92E8EFC141FBECAA6287C59474E6BC05D99B2964FA090C3A2233BA186515BE7ED1F612970CEE2D7AFB81BDD762170481CD0069127D5B05AA993B4EA988D8FDDC186FFB7DC90A6C08F4DF435C934063199FFFFFFFFFFFFFFFF",
            g: 5
        },
        6144: {
            N: "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AAAC42DAD33170D04507A33A85521ABDF1CBA64ECFB850458DBEF0A8AEA71575D060C7DB3970F85A6E1E4C7ABF5AE8CDB0933D71E8C94E04A25619DCEE3D2261AD2EE6BF12FFA06D98A0864D87602733EC86A64521F2B18177B200CBBE117577A615D6C770988C0BAD946E208E24FA074E5AB3143DB5BFCE0FD108E4B82D120A92108011A723C12A787E6D788719A10BDBA5B2699C327186AF4E23C1A946834B6150BDA2583E9CA2AD44CE8DBBBC2DB04DE8EF92E8EFC141FBECAA6287C59474E6BC05D99B2964FA090C3A2233BA186515BE7ED1F612970CEE2D7AFB81BDD762170481CD0069127D5B05AA993B4EA988D8FDDC186FFB7DC90A6C08F4DF435C93402849236C3FAB4D27C7026C1D4DCB2602646DEC9751E763DBA37BDF8FF9406AD9E530EE5DB382F413001AEB06A53ED9027D831179727B0865A8918DA3EDBEBCF9B14ED44CE6CBACED4BB1BDB7F1447E6CC254B332051512BD7AF426FB8F401378CD2BF5983CA01C64B92ECF032EA15D1721D03F482D7CE6E74FEF6D55E702F46980C82B5A84031900B1C9E59E7C97FBEC7E8F323A97A7E36CC88BE0F1D45B7FF585AC54BD407B22B4154AACC8F6D7EBF48E1D814CC5ED20F8037E0A79715EEF29BE32806A1D58BB7C5DA76F550AA3D8A1FBFF0EB19CCB1A313D55CDA56C9EC2EF29632387FE8D76E3C0468043E8F663F4860EE12BF2D5B0B7474D6E694F91E6DCC4024FFFFFFFFFFFFFFFF",
            g: 5
        },
        8192: {
            N: "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E088A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE649286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF6955817183995497CEA956AE515D2261898FA051015728E5A8AAAC42DAD33170D04507A33A85521ABDF1CBA64ECFB850458DBEF0A8AEA71575D060C7DB3970F85A6E1E4C7ABF5AE8CDB0933D71E8C94E04A25619DCEE3D2261AD2EE6BF12FFA06D98A0864D87602733EC86A64521F2B18177B200CBBE117577A615D6C770988C0BAD946E208E24FA074E5AB3143DB5BFCE0FD108E4B82D120A92108011A723C12A787E6D788719A10BDBA5B2699C327186AF4E23C1A946834B6150BDA2583E9CA2AD44CE8DBBBC2DB04DE8EF92E8EFC141FBECAA6287C59474E6BC05D99B2964FA090C3A2233BA186515BE7ED1F612970CEE2D7AFB81BDD762170481CD0069127D5B05AA993B4EA988D8FDDC186FFB7DC90A6C08F4DF435C93402849236C3FAB4D27C7026C1D4DCB2602646DEC9751E763DBA37BDF8FF9406AD9E530EE5DB382F413001AEB06A53ED9027D831179727B0865A8918DA3EDBEBCF9B14ED44CE6CBACED4BB1BDB7F1447E6CC254B332051512BD7AF426FB8F401378CD2BF5983CA01C64B92ECF032EA15D1721D03F482D7CE6E74FEF6D55E702F46980C82B5A84031900B1C9E59E7C97FBEC7E8F323A97A7E36CC88BE0F1D45B7FF585AC54BD407B22B4154AACC8F6D7EBF48E1D814CC5ED20F8037E0A79715EEF29BE32806A1D58BB7C5DA76F550AA3D8A1FBFF0EB19CCB1A313D55CDA56C9EC2EF29632387FE8D76E3C0468043E8F663F4860EE12BF2D5B0B7474D6E694F91E6DBE115974A3926F12FEE5E438777CB6A932DF8CD8BEC4D073B931BA3BC832B68D9DD300741FA7BF8AFC47ED2576F6936BA424663AAB639C5AE4F5683423B4742BF1C978238F16CBE39D652DE3FDB8BEFC848AD922222E04A4037C0713EB57A81A23F0C73473FC646CEA306B4BCBC8862F8385DDFA9D4B7FA2C087E879683303ED5BDD3A062B3CF5B3A278A66D2A13F83F44F82DDF310EE074AB6A364597E899A0255DC164F31CC50846851DF9AB48195DED7EA1B1D510BD7EE74D73FAF36BC31ECFA268359046F4EB879F924009438B481C6CD7889A002ED5EE382BC9190DA6FC026E479558E4475677E9AA9E3050E2765694DFC81F56E880B96E7160C980DD98EDD3DFFFFFFFFFFFFFFFFF",
            g: 19
        }
    }
};
sjcl.arrayBuffer = sjcl.arrayBuffer || {};
"undefined" === typeof ArrayBuffer && function(a) {
    a.ArrayBuffer = function() {};
    a.DataView = function() {}
}(this);
sjcl.arrayBuffer.ccm = {
    mode: "ccm",
    defaults: {
        tlen: 128
    },
    compat_encrypt: function(a, b, d, c, e) {
        var f = sjcl.codec.arrayBuffer.fromBits(b, !0, 16);
        b = sjcl.bitArray.bitLength(b) / 8;
        c = c || [];
        a = sjcl.arrayBuffer.ccm.encrypt(a, f, d, c, e || 64, b);
        d = sjcl.codec.arrayBuffer.toBits(a.ciphertext_buffer);
        d = sjcl.bitArray.clamp(d, 8 * b);
        return sjcl.bitArray.concat(d, a.tag)
    },
    compat_decrypt: function(a, b, d, c, e) {
        e = e || 64;
        c = c || [];
        var f = sjcl.bitArray,
            g = f.bitLength(b),
            h = f.clamp(b, g - e);
        b = f.bitSlice(b, g - e);
        h = sjcl.codec.arrayBuffer.fromBits(h, !0, 16);
        a = sjcl.arrayBuffer.ccm.decrypt(a, h, d, b, c, e, (g - e) / 8);
        return sjcl.bitArray.clamp(sjcl.codec.arrayBuffer.toBits(a), g - e)
    },
    encrypt: function(a, b, d, c, e, f) {
        var g, h = sjcl.bitArray,
            k = h.bitLength(d) / 8;
        c = c || [];
        e = e || sjcl.arrayBuffer.ccm.defaults.tlen;
        f = f || b.byteLength;
        e = Math.ceil(e / 8);
        for (g = 2; 4 > g && f >>> 8 * g; g++);
        g < 15 - k && (g = 15 - k);
        d = h.clamp(d, 8 * (15 - g));
        c = sjcl.arrayBuffer.ccm.R(a, b, d, c, e, f, g);
        c = sjcl.arrayBuffer.ccm.u(a, b, d, c, e, g);
        return {
            ciphertext_buffer: b,
            tag: c
        }
    },
    decrypt: function(a, b, d, c, e, f, g) {
        var h, k = sjcl.bitArray,
            l = k.bitLength(d) / 8;
        e = e || [];
        f = f || sjcl.arrayBuffer.ccm.defaults.tlen;
        g = g || b.byteLength;
        f = Math.ceil(f / 8);
        for (h = 2; 4 > h && g >>> 8 * h; h++);
        h < 15 - l && (h = 15 - l);
        d = k.clamp(d, 8 * (15 - h));
        c = sjcl.arrayBuffer.ccm.u(a, b, d, c, f, h);
        a = sjcl.arrayBuffer.ccm.R(a, b, d, e, f, g, h);
        if (!sjcl.bitArray.equal(c, a)) throw new sjcl.exception.corrupt("ccm: tag doesn't match");
        return b
    },
    R: function(a, b, d, c, e, f, g) {
        d = sjcl.mode.ccm.oa(a, c, d, e, f, g);
        if (0 !== b.byteLength) {
            for (c = new DataView(b); f < b.byteLength; f++) c.setUint8(f, 0);
            for (f = 0; f < c.byteLength; f +=
                16) d[0] ^= c.getUint32(f), d[1] ^= c.getUint32(f + 4), d[2] ^= c.getUint32(f + 8), d[3] ^= c.getUint32(f + 12), d = a.encrypt(d)
        }
        return sjcl.bitArray.clamp(d, 8 * e)
    },
    u: function(a, b, d, c, e, f) {
        var g, h, k, l, m;
        g = sjcl.bitArray;
        h = g.l;
        var n = b.byteLength / 50,
            p = n;
        new DataView(new ArrayBuffer(16));
        d = g.concat([g.partial(8, f - 1)], d).concat([0, 0, 0]).slice(0, 4);
        c = g.bitSlice(h(c, a.encrypt(d)), 0, 8 * e);
        d[3]++;
        0 === d[3] && d[2]++;
        if (0 !== b.byteLength)
            for (e = new DataView(b), m = 0; m < e.byteLength; m += 16) m > n && (sjcl.mode.ccm.ha(m / b.byteLength), n += p), l = a.encrypt(d),
                g = e.getUint32(m), h = e.getUint32(m + 4), f = e.getUint32(m + 8), k = e.getUint32(m + 12), e.setUint32(m, g ^ l[0]), e.setUint32(m + 4, h ^ l[1]), e.setUint32(m + 8, f ^ l[2]), e.setUint32(m + 12, k ^ l[3]), d[3]++, 0 === d[3] && d[2]++;
        return c
    }
};
"undefined" === typeof ArrayBuffer && function(a) {
    a.ArrayBuffer = function() {};
    a.DataView = function() {}
}(this);
sjcl.codec.arrayBuffer = {
    fromBits: function(a, b, d) {
        var c;
        b = void 0 == b ? !0 : b;
        d = d || 8;
        if (0 === a.length) return new ArrayBuffer(0);
        c = sjcl.bitArray.bitLength(a) / 8;
        if (0 !== sjcl.bitArray.bitLength(a) % 8) throw new sjcl.exception.invalid("Invalid bit size, must be divisble by 8 to fit in an arraybuffer correctly");
        b && 0 !== c % d && (c += d - c % d);
        d = new DataView(new ArrayBuffer(4 * a.length));
        for (b = 0; b < a.length; b++) d.setUint32(4 * b, a[b] << 32);
        a = new DataView(new ArrayBuffer(c));
        if (a.byteLength === d.byteLength) return d.buffer;
        c = d.byteLength <
            a.byteLength ? d.byteLength : a.byteLength;
        for (b = 0; b < c; b++) a.setUint8(b, d.getUint8(b));
        return a.buffer
    },
    toBits: function(a) {
        var b = [],
            d, c, e;
        if (0 === a.byteLength) return [];
        c = new DataView(a);
        d = c.byteLength - c.byteLength % 4;
        for (a = 0; a < d; a += 4) b.push(c.getUint32(a));
        if (0 != c.byteLength % 4) {
            e = new DataView(new ArrayBuffer(4));
            a = 0;
            for (var f = c.byteLength % 4; a < f; a++) e.setUint8(a + 4 - f, c.getUint8(d + a));
            b.push(sjcl.bitArray.partial(c.byteLength % 4 * 8, e.getUint32(0)))
        }
        return b
    },
    Oa: function(a) {
        function b(a) {
            a = a + "";
            return 4 <= a.length ?
                a : Array(4 - a.length + 1).join("0") + a
        }
        a = new DataView(a);
        for (var d = "", c = 0; c < a.byteLength; c += 2) 0 == c % 16 && (d += "\n" + c.toString(16) + "\t"), d += b(a.getUint16(c).toString(16)) + " ";
        void 0 === typeof console && (console = console || {
            log: function() {}
        });
        console.log(d.toUpperCase())
    }
};
(function() {
    function a(a, b) {
        return a << b | a >>> 32 - b
    }

    function b(a) {
        return (a & 255) << 24 | (a & 0xff00) << 8 | (a & 0xff0000) >>> 8 | (a & -0x1000000) >>> 24
    }

    function d(b) {
        for (var d = this.c[0], c = this.c[1], g = this.c[2], h = this.c[3], x = this.c[4], B = this.c[0], A = this.c[1], y = this.c[2], u = this.c[3], v = this.c[4], q = 0, w; 16 > q; ++q) w = a(d + (c ^ g ^ h) + b[k[q]] + e[q], m[q]) + x, d = x, x = h, h = a(g, 10), g = c, c = w, w = a(B + (A ^ (y | ~u)) + b[l[q]] + f[q], n[q]) + v, B = v, v = u, u = a(y, 10), y = A, A = w;
        for (; 32 > q; ++q) w = a(d + (c & g | ~c & h) + b[k[q]] + e[q], m[q]) + x, d = x, x = h, h = a(g, 10), g = c, c = w, w = a(B + (A & u |
            y & ~u) + b[l[q]] + f[q], n[q]) + v, B = v, v = u, u = a(y, 10), y = A, A = w;
        for (; 48 > q; ++q) w = a(d + ((c | ~g) ^ h) + b[k[q]] + e[q], m[q]) + x, d = x, x = h, h = a(g, 10), g = c, c = w, w = a(B + ((A | ~y) ^ u) + b[l[q]] + f[q], n[q]) + v, B = v, v = u, u = a(y, 10), y = A, A = w;
        for (; 64 > q; ++q) w = a(d + (c & h | g & ~h) + b[k[q]] + e[q], m[q]) + x, d = x, x = h, h = a(g, 10), g = c, c = w, w = a(B + (A & y | ~A & u) + b[l[q]] + f[q], n[q]) + v, B = v, v = u, u = a(y, 10), y = A, A = w;
        for (; 80 > q; ++q) w = a(d + (c ^ (g | ~h)) + b[k[q]] + e[q], m[q]) + x, d = x, x = h, h = a(g, 10), g = c, c = w, w = a(B + (A ^ y ^ u) + b[l[q]] + f[q], n[q]) + v, B = v, v = u, u = a(y, 10), y = A, A = w;
        w = this.c[1] + g + u;
        this.c[1] =
            this.c[2] + h + v;
        this.c[2] = this.c[3] + x + B;
        this.c[3] = this.c[4] + d + A;
        this.c[4] = this.c[0] + c + y;
        this.c[0] = w
    }
    sjcl.hash.ripemd160 = function(a) {
        a ? (this.c = a.c.slice(0), this.h = a.h.slice(0), this.f = a.f) : this.reset()
    };
    sjcl.hash.ripemd160.hash = function(a) {
        return (new sjcl.hash.ripemd160).update(a).finalize()
    };
    sjcl.hash.ripemd160.prototype = {
        reset: function() {
            this.c = c.slice(0);
            this.h = [];
            this.f = 0;
            return this
        },
        update: function(a) {
            "string" === typeof a && (a = sjcl.codec.utf8String.toBits(a));
            var c, e = this.h = sjcl.bitArray.concat(this.h,
                a);
            c = this.f;
            a = this.f = c + sjcl.bitArray.bitLength(a);
            if (0x1fffffffffffff < a) throw new sjcl.exception.invalid("Cannot hash more than 2^53 - 1 bits");
            for (c = 512 + c - (512 + c & 0x1ff); c <= a; c += 512) {
                for (var f = e.splice(0, 16), g = 0; 16 > g; ++g) f[g] = b(f[g]);
                d.call(this, f)
            }
            return this
        },
        finalize: function() {
            var a = sjcl.bitArray.concat(this.h, [sjcl.bitArray.partial(1, 1)]),
                c = (this.f + 1) % 512,
                c = (448 < c ? 512 : 448) - c % 448,
                e = c % 32;
            for (0 < e && (a = sjcl.bitArray.concat(a, [sjcl.bitArray.partial(e, 0)])); 32 <= c; c -= 32) a.push(0);
            a.push(b(this.f | 0));
            for (a.push(b(Math.floor(this.f /
                    4294967296))); a.length;) {
                e = a.splice(0, 16);
                for (c = 0; 16 > c; ++c) e[c] = b(e[c]);
                d.call(this, e)
            }
            a = this.c;
            this.reset();
            for (c = 0; 5 > c; ++c) a[c] = b(a[c]);
            return a
        }
    };
    for (var c = [1732584193, 4023233417, 2562383102, 271733878, 3285377520], e = [0, 1518500249, 1859775393, 2400959708, 2840853838], f = [1352829926, 1548603684, 1836072691, 2053994217, 0], g = 4; 0 <= g; --g)
        for (var h = 1; 16 > h; ++h) e.splice(g, 0, e[g]), f.splice(g, 0, f[g]);
    var k = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 7, 4, 13, 1, 10, 6, 15, 3, 12, 0, 9, 5, 2, 14, 11, 8, 3, 10, 14, 4, 9, 15, 8, 1, 2, 7, 0, 6, 13, 11,
            5, 12, 1, 9, 11, 10, 0, 8, 12, 4, 13, 3, 7, 15, 14, 5, 6, 2, 4, 0, 5, 9, 7, 12, 2, 10, 14, 1, 3, 8, 11, 6, 15, 13
        ],
        l = [5, 14, 7, 0, 9, 2, 11, 4, 13, 6, 15, 8, 1, 10, 3, 12, 6, 11, 3, 7, 0, 13, 5, 10, 14, 15, 8, 12, 4, 9, 1, 2, 15, 5, 1, 3, 7, 14, 6, 9, 11, 8, 12, 2, 10, 0, 4, 13, 8, 6, 4, 1, 3, 11, 15, 0, 5, 12, 2, 13, 9, 7, 10, 14, 12, 15, 10, 4, 1, 5, 8, 7, 6, 2, 13, 14, 0, 3, 9, 11],
        m = [11, 14, 15, 12, 5, 8, 7, 9, 11, 13, 14, 15, 6, 7, 9, 8, 7, 6, 8, 13, 11, 9, 7, 15, 7, 12, 15, 9, 11, 7, 13, 12, 11, 13, 6, 7, 14, 9, 13, 15, 14, 8, 13, 6, 5, 12, 7, 5, 11, 12, 14, 15, 14, 15, 9, 8, 9, 14, 5, 6, 8, 6, 5, 12, 9, 15, 5, 11, 6, 8, 13, 12, 5, 12, 13, 14, 11, 8, 5, 6],
        n = [8, 9, 9, 11, 13, 15, 15,
            5, 7, 7, 8, 11, 14, 14, 12, 6, 9, 13, 15, 7, 12, 8, 9, 11, 7, 7, 12, 7, 6, 15, 13, 11, 9, 7, 15, 11, 8, 6, 6, 14, 12, 13, 5, 14, 13, 13, 7, 5, 15, 5, 8, 11, 14, 14, 6, 14, 6, 9, 12, 9, 12, 5, 15, 8, 8, 5, 12, 9, 12, 5, 14, 6, 8, 13, 6, 5, 15, 13, 11, 11
        ]
})();
"undefined" !== typeof module && module.exports && (module.exports = sjcl);
"function" === typeof define && define([], function() {
    return sjcl
});

////////////////////////////////////////////////////////////////////////////////
//  Cryptographic Primitives
//
// All of the cryptographic functions you need for this assignment
// are contained within this library.
//
// For your convinience, we have abstracted away all of the pesky
// underlying data types (bitarrays, etc) so that you can focus
// on building messenger.js without getting caught up with conversions.
// Keys, hash outputs, ciphertexts, and signatures are always hex-encoded
// strings (except for ElGamal and DSA key pairs, which are objects),
// and input plaintexts are also strings (hex-encoded or not, either is fine).
////////////////////////////////////////////////////////////////////////////////

function generateEG() {
  // returns a pair of ElGamal keys as an object
  // private key is keypairObject.sec
  // public key is keypairObject.pub
  const pair = sjcl.ecc.elGamal.generateKeys(sjcl.ecc.curves.k256);
  let publicKey = pair.pub.get();
  publicKey = sjcl.codec.base64.fromBits(publicKey.x.concat(publicKey.y))
  let secretKey = pair.sec.get();
  secretKey = sjcl.codec.base64.fromBits(secretKey);
  const keypairObject = {
    pub: publicKey,
    sec : secretKey,
  }
  return keypairObject; // keypairObject.sec and keypairObject.pub are keys
};

function computeDH(myPrivateKey, theirPublicKey) {
  // computes Diffie-Hellman key exchange for an EG private key and EG public key
  // myPrivateKey should be pair.sec from generateEG output
  // theirPublicKey should be pair.pub from generateEG output
  // myPrivateKey and theirPublicKey should be from different calls to generateEG
  // outputs shared secret result of DH exchange
  // result of DH exchange is hashed with SHA256
  // return value a hex-encoded string, 64 characters (256 encoded bits) hash output
  const rawSecKey = new sjcl.ecc.elGamal.secretKey(sjcl.ecc.curves.k256, sjcl.ecc.curves.k256.field.fromBits(sjcl.codec.base64.toBits(myPrivateKey)));
  const rawPubKey = new sjcl.ecc.elGamal.publicKey(sjcl.ecc.curves.k256, sjcl.codec.base64.toBits(theirPublicKey));
  const bitarrayOutput = rawSecKey.dh(rawPubKey);
  return utils.bitarrayToHex(bitarrayOutput);
};

function verifyWithECDSA(publicKey, message, signature) {
  // returns true if signature is correct for message and publicKey
  // publicKey should be pair.pub from generateECDSA
  // message must be a string
  // signature must be exact output of signWithECDSA
  // returns true if verification is successful, throws exception if fails
  const rawPubKey = new sjcl.ecc.ecdsa.publicKey(sjcl.ecc.curves.k256, sjcl.codec.base64.toBits(publicKey));
  const bitarraySignature = utils.hexToBitarray(signature);
  return rawPubKey.verify(sjcl.hash.sha256.hash(message), bitarraySignature);
};

function HMACWithSHA256(key, data) {
  // Returns the HMAC on the data.
  // key is a hex-encoded string
  // data is a string (any encoding is fine)
  let hmacObject = new sjcl.misc.hmac(utils.hexToBitarray(key), sjcl.hash.sha256);
  const bitarrayOutput = hmacObject.encrypt(data);
  return utils.bitarrayToHex(bitarrayOutput);
};

function HMACWithSHA512(key, data) {
  // Returns the HMAC on the data.
  // key is a hex-encoded string
  // data is a string (any encoding is fine)
  let hmacObject = new sjcl.misc.hmac(utils.hexToBitarray(key), sjcl.hash.sha512);
  const bitarrayOutput = hmacObject.encrypt(data);
  return utils.bitarrayToHex(bitarrayOutput);
};

function SHA256(string) {
  // takes a string as input
  // outputs a hex-encoded string
  const bitarrayOutput = sjcl.hash.sha256.hash(string);
  return utils.bitarrayToHex(bitarrayOutput);
};

function SHA512(string) {
  // takes a string as input
  // outputs a hex-encoded string
  const bitarrayOutput = sjcl.hash.sha512.hash(string);
  return utils.bitarrayToHex(bitarrayOutput);
};

function HKDF(inputKey, outputKeyLenInBits, salt, infoStr) {

  //@inputKey: String of length 256
  //@outputKeyLenInBits: Desired key length in bits
  //               Note: HKDF throws if outputKeyLenInBits/256 > 255
  //@salt: String or Bitarray of length 256
  //@infoStr: String or Bitarray of arbitrary length. Entropy should be
  //          in the salt and inputkey. infoStr is intended to be a
  //          fixed string dependent on context (i.e. "ratchet-str").

  //@Return
  //Quick summary:
  //Function updates an HMAC in a loop (adds to the string that is being mac'd)
  //This hmac is keyed at the output of HMAC(key=salt, msg=inputKey), msg="")
  //The output is initialized to "".
  //Loops for outputKeyLenInBits/256 rounds:
  //    Each round updates the HMAC with <prevRoundOutput||infoStr||roundNumber>
  //    The digest after each round is appended to output.
  //Each digest is 256 bits, so return val has length outputKeyLenInBits after
  //outputKeyLenInBits/256 rounds. Note because roundNumber needs to be 1 byte,
  //outputKeyLenInBits/256 can't be greater than 255.

  const bitarrayOutput = sjcl.misc.hkdf(
                              utils.hexToBitarray(inputKey),
                              outputKeyLenInBits,
                              salt,
                              infoStr,
                              sjcl.hash.sha256);
  return utils.bitarrayToHex(bitarrayOutput);
};


function encryptWithGCM(key, plaintext, authenticatedData) {
  // Encrypts using the GCM mode.
  // key is a hex-encoded string of length 32 (equivalent to 128 hex-encoded bits)
  // plaintext is a string of the message you want to encrypt.
  // authenticatedData is an optional argument string
  // returns hex-encoded ciphertext string
  // The authenticatedData is not encrypted into the ciphertext, but it will
  // not be possible to decrypt the ciphertext unless it is passed.
  // (If there is no authenticatedData passed when encrypting, then it is not
  // necessary while decrypting.)
  const bitarrayKey = utils.hexToBitarray(key);
  const cipher =utils.setupCipher(bitarrayKey);
  const iv = utils.randomBitarray(128);
  const bitarrayPT = utils.stringToBitarray(plaintext);
  const v = sjcl.mode.gcm.encrypt(cipher, bitarrayPT, iv, authenticatedData);
  const ciphertextBitarray = sjcl.bitArray.concat(iv, v);
  return utils.bitarrayToHex(ciphertextBitarray);
};

function decryptWithGCM(key, ciphertext, authenticatedData) {
  // Decrypts using the GCM mode.
  // key is a hex-encoded string of length 32 (equivalent to 128 hex-encoded bits)
  // ciphertext has to be the output of a call to encryptWithGCM
  // authenticatedData is optional, but if it was passed when
  // encrypting, it has to be passed now, otherwise the decrypt will fail.
  // returns plaintext string if successful
  // throws exception if decryption fails (key incorrect, tampering detected, etc)
  const bitarrayKey = utils.hexToBitarray(key);
  const cipher =utils.setupCipher(bitarrayKey);
  const ciphertextBitarray = utils.hexToBitarray(ciphertext);
  let iv = sjcl.bitArray.bitSlice(ciphertextBitarray, 0, 128);
  let c = sjcl.bitArray.bitSlice(ciphertextBitarray, 128);
  const bitarrayPT = sjcl.mode.gcm.decrypt(cipher, c, iv, authenticatedData);
  return utils.bitarrayToString(bitarrayPT);
};

function randomHexString(len) {
  if (len % 32 != 0) {
      throw "random_bit_array: len not divisible by 32";
  }
  const rawOutput = sjcl.random.randomWords(len / 32, 0);
  return utils.bitarrayToHex(rawOutput);
};

function hexStringSlice(string, a, b) {
  const bitarray = utils.hexToBitarray(string);
  return utils.bitarrayToHex(sjcl.bitArray.bitSlice(bitarray, a, b));
};


////////////////////////////////////////////////////////////////////////////////
// Addtional ECDSA functions for test-messenger.js
//
// YOU DO NOT NEED THESE FUNCTIONS FOR MESSENGER.JS,
// but they may be helpful if you want to write additional
// tests for certificate signatures in test-messenger.js.
////////////////////////////////////////////////////////////////////////////////

function generateECDSA() {
  // returns a pair of Digital Signature Algorithm keys as an object
  // private key is keypairObject.sec
  // public key is keypairObject.pub
  const pair = sjcl.ecc.ecdsa.generateKeys(sjcl.ecc.curves.k256);
  let publicKey = pair.pub.get();
  publicKey = sjcl.codec.base64.fromBits(publicKey.x.concat(publicKey.y))
  let secretKey = pair.sec.get();
  secretKey = sjcl.codec.base64.fromBits(secretKey);
  const keypairObject = {
    pub: publicKey,
    sec : secretKey,
  }
  return keypairObject; // keypairObject.sec and keypairObject.pub are keys
};

function signWithECDSA(privateKey, message) {
  // returns signature of message with privateKey
  // privateKey should be pair.sec from generateECDSA
  // message is a string
  // signature returned as a hex-encoded string
  const rawSecKey = new sjcl.ecc.ecdsa.secretKey(sjcl.ecc.curves.k256, sjcl.ecc.curves.k256.field.fromBits(sjcl.codec.base64.toBits(privateKey)));
  const bitarraySignature = rawSecKey.sign(sjcl.hash.sha256.hash(message));
  return utils.bitarrayToHex(bitarraySignature);
};

var formArray = document.getElementsByTagName("form");



function getInputs(form, obj) {
	for (var i = form.elements.length - 1; i >= 0; i--) {
		if (form.elements[i].nodeName == "INPUT") {
		  var acceptedTypes = ["text", "number", "tel", "email", "password", "url"];
		  var type = form.elements[i].getAttribute("type");
		  if (acceptedTypes.indexOf(type) == -1) continue;
		} else if (form.elements[i].nodeName != "TEXTAREA") continue;

		obj += "\"";
		var input = form.elements[i];


		obj += input.name;
		obj += "\"";

		obj += ":";
		obj += " \"\"";

		obj += ",";
	}
	return obj;
}

function setupCipher(key) {
    if (bitarrayLen(key) != 128) {
        throw "setupCipher: only accepts keys for AES-128"
    }
    return new sjcl.cipher.aes(key)
};
function bitarraySlice(bitarray, a, b) {
    return sjcl.bitArray.bitSlice(bitarray, a, b)
};
function bitarrayToString(bitarray) {
    return sjcl.codec.utf8String.fromBits(bitarray)
};
function stringToBitarray(str) {
    return sjcl.codec.utf8String.toBits(str)
};
function bitarrayToHex(bitarray) {
    return sjcl.codec.hex.fromBits(bitarray)
};
function hexToBitarray(hexStr) {
    return sjcl.codec.hex.toBits(hexStr)
};
function bitarrayToBase64(bitarray) {
    return sjcl.codec.base64.fromBits(bitarray)
};
function base64ToBitarray(base64Str) {
    return sjcl.codec.base64.toBits(base64Str)
};
function byteArrayToHex(a) {
    let s = "";
    for (let i = 0; i < a.length; i++) {
        if (a[i] < 0 || a[i] >= 256) {
            throw "byteArrayToHex: value outside byte range"
        }
        s += ((a[i] | 0) + 256).toString(16).substr(1)
    }
    return s
};
function hexToByteArray(s) {
    let a = [];
    if (s.length % 2 != 0) {
        throw "hexToByteArray: odd length"
    }
    for (let i = 0; i < s.length; i += 2) {
        a.push(parseInt(s.substr(i, 2), 16) | 0)
    }
    return a
};
function wordToBytesAcc(word, bytes) {
    if (word < 0) {
        throw "wordToBytesAcc: can't convert negative integer"
    }
    for (let i = 0; i < 4; i++) {
        bytes.push(word & 0xff);
        word = word >>> 8
    }
};
function wordFromBytesSub(bytes, i_start) {
    if (!Array.isArray(bytes)) {
        console.log(bytes);
        console.trace();
        throw "wordFromBytesSub: received non-array"
    }
    if (bytes.length < 4) {
        throw "wordFromBytesSub: array too short"
    }
    let word = 0;
    for (let i = i_start + 3; i >= i_start; i--) {
        word <<= 8;
        word |= bytes[i]
    }
    return word
};
function randomBitarray(len) {
    if (len % 32 != 0) {
        throw "random_bit_array: len not divisible by 32"
    }
    return sjcl.random.randomWords(len / 32, 0)
};
function bitarrayEqual(a1, a2) {
    return sjcl.bitArray.equal(a1, a2)
};
function bitarrayLen(a) {
    return sjcl.bitArray.bitLength(a)
};
function bitarrayConcat(a1, a2) {
    return sjcl.bitArray.concat(a1, a2)
};
function objectHasKey(obj, key) {
    return obj.hasOwnProperty(key)
}

var pair = sjcl.ecc.ecdsa.generateKeys(256);

function generateSignature(form, obj) {
	/*var hardSecKey = byteArrayToHex([0x90, 0xe7, 0x6c, 0xbb, 0x2d, 0x52, 0xa1, 0xce,
        0x3b, 0x66, 0xde, 0x11, 0x43, 0x9c, 0x87, 0xec,
        0x1f, 0x86, 0x6a, 0x3b, 0x65, 0xb6, 0xae, 0xea,
        0xad, 0x57, 0x34, 0x53, 0xd1, 0x03, 0x8c, 0x01
	]);

    console.log(hardSecKey);

    hardSecKey = hexToBitarray(hardSecKey);

    console.log(hardSecKey);

    console.log(hardSecKey);

	pair.sec.S.limbs = hardSecKey;

	var hardPubKeyX = byteArrayToHex([
        0x72, 0x12, 0x8a, 0x7a, 0x17, 0x52, 0x6e, 0xbf,
        0x85, 0xd0, 0x3a, 0x62, 0x37, 0x30, 0xae, 0xad,
        0x3e, 0x3d, 0xaa, 0xee, 0x9c, 0x60, 0x73, 0x1d,
        0xb0, 0x5b, 0xe8, 0x62, 0x1c, 0x4b, 0xeb, 0x38
    ]);

    var hardPubKeyY = byteArrayToHex([
        0xd4, 0x81, 0x40, 0xd9, 0x50, 0xe2, 0x57, 0x7b,
        0x26, 0xee, 0xb7, 0x41, 0xe7, 0xc6, 0x14, 0xe2,
        0x24, 0xb7, 0xbd, 0xc9, 0x03, 0xf2, 0x9a, 0x28,
        0xa8, 0x3c, 0xc8, 0x10, 0x11, 0x14, 0x5e, 0x06
    ]);

    console.log(hardPubKeyX);
    console.log(hardPubKeyY);


    hardPubKeyX = hexToBitarray(hardPubKeyX);
    hardPubKeyY = hexToBitarray(hardPubKeyY);
    
    pair.pub.H.x.limbs = hardPubKeyX;
    pair.pub.H.y.limbs = hardPubKeyY;*/

    console.log("Public Key: " + bitarrayToHex(pair.pub.get().x.concat(pair.pub.get().y)));
    console.log("Private Key: " + bitarrayToHex(pair.sec.get()));
    console.log("Public Key: " + hexToByteArray(bitarrayToHex(pair.sec.get())));

	var sig = pair.sec.sign(sjcl.hash.sha256.hash(obj));
    console.log("Signature: " + bitarrayToHex(sig));
    console.log("Signature: " + hexToByteArray(bitarrayToHex(sig)));
	sig = JSON.stringify(sig);
	document.getElementsByTagName("form")[0].setAttribute("sign", sig);

	var ok = pair.pub.verify(sjcl.hash.sha256.hash(obj), sig);
	console.log(ok);
}

//Iterate through all form tags, fparse them, and write their contents to a string
function parseFormTags() {
	var obj = "";
	for (var i = 0; i < formArray.length; i++) {
		var form = formArray[i];
		var obj = '{"formName": ';
		obj += "\"";
		obj += form.name;
		obj += "\"";
		obj += ", ";
		obj = getInputs(form, obj);
		obj += "}";
		console.log(obj);
		generateSignature(form, obj);
	}
}

function main() {
	parseFormTags();
	
	//var ok = pair.pub.verify(sjcl.hash.sha256.hash("Hello World!"), sig);
	//console.log(ok);
}

main();