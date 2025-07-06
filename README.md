🧠 Design Philosophy: Why Kernel/ and Core/ Directories?
The natGhost module is divided into two main interconnected components:

Kernel/: Written in low-level C (bare-metal or kernelspace logic)

Core/: Written in Python for high-level experimentation and research

This separation is not architectural preference, but a technical necessity.

Unlike high-level OOP languages (such as Python), low-level languages like C/C++ allow us to bypass user-space restrictions and access hardware resources directly — especially when implementing techniques such as kernel bypass, raw socket forging, or NAT traversal bursts.

As a result, all burst-related, raw socket-based, or performance-critical mechanisms reside in the Kernel/ directory, while the analysis, orchestration, and experimentation layers are implemented in Python under Core/.

🔍 Philosophy of natGhost: Beyond a Module
natGhost is not a standalone project.
It is a necessary submodule of the SafeRoom system, developed out of real-world constraints encountered during peer-to-peer (P2P) communication research.

Its purpose is to achieve the fastest and most efficient NAT traversal solutions using the least amount of system resources.

The techniques developed here are:

Highly pragmatic

Designed based on actual network topologies and constraints

Continuously tested under diverse NAT environments

Once the research phase is complete, the findings and protocols will be formally documented and submitted to the Internet Engineering Task Force (IETF).

🔓 Open Source Ethics
Both natGhost and SafeRoom are open-source and anti-proprietary in philosophy.
While the final products may be commercialized, the underlying technologies will never be.

This module strongly supports the open-source community and believes that technology must remain uncaged.

⚠️ Technological Significance
This repository includes groundbreaking methods to overcome what is considered the "Three-Body Problem" of the networking world:

Port-Restricted Cone NAT

Symmetric NAT

Symmetric NATs, which are theoretically and practically seen as “undrillable”, have been consistently bypassed using advanced, recursive strategies designed within this framework.

Therefore, natGhost is not just a module — it is a catalyst for real-time, relayless P2P communication technology.

Performance Comparison vs ICE
While ICE (Interactive Connectivity Establishment) is widely used for NAT traversal in real-time systems, it falls short in several key areas:

| Feature                    | ICE Protocol        | `natGhost` Module        |
| -------------------------- | ------------------- | ------------------------ |
| STUN Server Selection Time | 500–1500 ms (avg)   | **29–132 ms** (measured) |
| Symmetric NAT Support      | ❌ Not functional    | ✅ High success rate    |
| Retry Overhead             | 2–3 retries typical | ❌ None                  |
| Resource Usage             | Medium to High      | **Very Low**             |

In our real-world observations, ICE often took 500–1500 ms to select a reflexive (STUN) candidate, depending on retry attempts and STUN response delays (50–200 ms).
natGhost, by contrast, completes this entire process in ~100 ms on average, with:

Best case: 29 ms

Worst case: 132 ms

Even in the worst-case scenario, natGhost outperforms ICE’s best-case by a factor of 3.78x.

