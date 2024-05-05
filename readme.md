# HIPS (Header-only IPS)

A C++ single-header library for applying IPS, UPS and BPS patches, with an easy-to-use API and error checking.

Example for applying a BPS patch:
```cc
// Returns an std::vector<u8> containing the patched file (bytes) and an enum for reporting any errors (invalid patch, checksum mismatches, etc)

auto [bytes, result] = Hips::patch(inputData, inputSize, patchData, patchSize, Hips::PatchType::BPS);
```

For a full example, check out the examples folder