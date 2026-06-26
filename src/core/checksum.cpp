#include "mercury/core.hpp"

#include <array>
#include <iomanip>
#include <sstream>

namespace mercury {

namespace {

constexpr std::array<std::uint32_t, 64> kSha256RoundConstants{
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u,
    0xab1c5ed5u, 0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu,
    0x9bdc06a7u, 0xc19bf174u, 0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu,
    0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau, 0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u,
    0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u, 0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu,
    0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u, 0xa2bfe8a1u, 0xa81a664bu,
    0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u, 0x19a4c116u,
    0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u,
    0xc67178f2u};

std::uint32_t rotate_right(std::uint32_t value, int count) {
  return (value >> count) | (value << (32 - count));
}

std::uint32_t read_be32(const std::uint8_t* bytes) {
  return (static_cast<std::uint32_t>(bytes[0]) << 24) | (static_cast<std::uint32_t>(bytes[1]) << 16) |
         (static_cast<std::uint32_t>(bytes[2]) << 8) | static_cast<std::uint32_t>(bytes[3]);
}

void write_be32(std::uint32_t value, std::uint8_t* out) {
  out[0] = static_cast<std::uint8_t>(value >> 24);
  out[1] = static_cast<std::uint8_t>(value >> 16);
  out[2] = static_cast<std::uint8_t>(value >> 8);
  out[3] = static_cast<std::uint8_t>(value);
}

void sha256_transform(std::array<std::uint32_t, 8>& state, const std::uint8_t* block) {
  std::array<std::uint32_t, 64> schedule{};
  for (std::size_t i = 0; i < 16; ++i) {
    schedule[i] = read_be32(block + i * 4);
  }
  for (std::size_t i = 16; i < schedule.size(); ++i) {
    const auto s0 = rotate_right(schedule[i - 15], 7) ^ rotate_right(schedule[i - 15], 18) ^ (schedule[i - 15] >> 3);
    const auto s1 = rotate_right(schedule[i - 2], 17) ^ rotate_right(schedule[i - 2], 19) ^ (schedule[i - 2] >> 10);
    schedule[i] = schedule[i - 16] + s0 + schedule[i - 7] + s1;
  }

  auto a = state[0];
  auto b = state[1];
  auto c = state[2];
  auto d = state[3];
  auto e = state[4];
  auto f = state[5];
  auto g = state[6];
  auto h = state[7];

  for (std::size_t i = 0; i < schedule.size(); ++i) {
    const auto s1 = rotate_right(e, 6) ^ rotate_right(e, 11) ^ rotate_right(e, 25);
    const auto choose = (e & f) ^ (~e & g);
    const auto temp1 = h + s1 + choose + kSha256RoundConstants[i] + schedule[i];
    const auto s0 = rotate_right(a, 2) ^ rotate_right(a, 13) ^ rotate_right(a, 22);
    const auto majority = (a & b) ^ (a & c) ^ (b & c);
    const auto temp2 = s0 + majority;

    h = g;
    g = f;
    f = e;
    e = d + temp1;
    d = c;
    c = b;
    b = a;
    a = temp1 + temp2;
  }

  state[0] += a;
  state[1] += b;
  state[2] += c;
  state[3] += d;
  state[4] += e;
  state[5] += f;
  state[6] += g;
  state[7] += h;
}

} // namespace

std::uint32_t crc32(std::span<const std::uint8_t> bytes) {
  std::uint32_t crc = 0xffffffffu;
  for (std::uint8_t byte : bytes) {
    crc ^= byte;
    for (int bit = 0; bit < 8; ++bit) {
      const std::uint32_t mask = -(crc & 1u);
      crc = (crc >> 1u) ^ (0xedb88320u & mask);
    }
  }
  return ~crc;
}

std::uint64_t crc64_ecma(std::span<const std::uint8_t> bytes) {
  std::uint64_t crc = 0;
  for (std::uint8_t byte : bytes) {
    crc ^= static_cast<std::uint64_t>(byte) << 56;
    for (int bit = 0; bit < 8; ++bit) {
      if ((crc & 0x8000000000000000ull) != 0) {
        crc = (crc << 1) ^ 0x42f0e1eba9ea3693ull;
      } else {
        crc <<= 1;
      }
    }
  }
  return crc;
}

std::uint32_t fnv1a32(std::span<const std::uint8_t> bytes) {
  std::uint32_t hash = 2166136261u;
  for (std::uint8_t byte : bytes) {
    hash ^= byte;
    hash *= 16777619u;
  }
  return hash;
}

std::array<std::uint8_t, 32> sha256(std::span<const std::uint8_t> bytes) {
  std::array<std::uint32_t, 8> state{0x6a09e667u, 0xbb67ae85u, 0x3c6ef372u, 0xa54ff53au,
                                    0x510e527fu, 0x9b05688cu, 0x1f83d9abu, 0x5be0cd19u};

  const auto full_blocks = bytes.size() / 64;
  for (std::size_t i = 0; i < full_blocks; ++i) {
    sha256_transform(state, bytes.data() + i * 64);
  }

  std::array<std::uint8_t, 128> tail{};
  const auto remainder = bytes.size() % 64;
  for (std::size_t i = 0; i < remainder; ++i) {
    tail[i] = bytes[full_blocks * 64 + i];
  }
  tail[remainder] = 0x80;

  const auto bit_length = static_cast<std::uint64_t>(bytes.size()) * 8;
  const std::size_t padded_size = remainder < 56 ? 64 : 128;
  for (std::size_t i = 0; i < 8; ++i) {
    tail[padded_size - 1 - i] = static_cast<std::uint8_t>(bit_length >> (i * 8));
  }

  sha256_transform(state, tail.data());
  if (padded_size == 128) {
    sha256_transform(state, tail.data() + 64);
  }

  std::array<std::uint8_t, 32> digest{};
  for (std::size_t i = 0; i < state.size(); ++i) {
    write_be32(state[i], digest.data() + i * 4);
  }
  return digest;
}

std::string hex_digest(std::span<const std::uint8_t> bytes) {
  std::ostringstream out;
  out << std::hex << std::setfill('0');
  for (std::uint8_t byte : bytes) {
    out << std::setw(2) << static_cast<unsigned>(byte);
  }
  return out.str();
}

} // namespace mercury
