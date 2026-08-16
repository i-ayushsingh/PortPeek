# PortPeek Website Specification

> **Status:** Complete website redesign specification  
> **Product:** PortPeek  
> **Repository:** `i-ayushsingh/PortPeek`  
> **Website:** GitHub Pages from `/docs`  
> **Implementation:** Vanilla HTML5 + CSS3 + modern JavaScript  
> **Primary platform:** Windows 11  
> **Primary conversion:** Download PortPeek  
> **Secondary conversion:** GitHub / Explore / Learn

---

# 1. Product Definition

## 1.1 What is PortPeek?

PortPeek is an ultra-lightweight native Windows system-tray utility for developers who work with local development servers.

Its core job is simple:

> **See what's running locally and get to it instantly.**

PortPeek detects active local development ports, identifies the owning process and, where supported by the application, identifies the framework/service associated with the port.

The user can open a local service directly from the tray without manually typing localhost URLs or running terminal commands to discover which process owns a port.

The current project specification describes PortPeek as a native Win32 C++17 application with an approximately **500 KB** standalone executable target, very low idle resource usage, no Electron/WebView dependency, Windows-native behavior, and local-only operation.

Do not use the outdated `~340 KB` figure from the previous website specification.

---

# 2. Website Mission

The website has two completely different jobs.

## Job A — Sell PortPeek

The landing page must make a developer understand PortPeek extremely quickly.

Within roughly 10 seconds, a visitor should understand:

1. What PortPeek does.
2. Why it is useful.
3. What it looks like.
4. That it is native to Windows.
5. That it is extremely lightweight.
6. That it is open source.
7. Where to download it.

The primary CTA is:

> **Download for Windows**

The GitHub repository is the secondary trust/action destination.

---

## Job B — Become a serious developer resource

The rest of the website should grow into a high-quality resource around:

- Ports
- Localhost
- TCP
- UDP
- Windows networking
- Development servers
- Framework development ports
- Databases
- AI/LLM local servers
- Docker
- Troubleshooting
- `EADDRINUSE`
- Process/port relationships
- Windows development workflows

This content must not be stuffed into the landing page.

The landing page is short.

The knowledge system is deep.

---

# 3. Core Design Philosophy

PortPeek's website must NOT look like a generic AI-generated SaaS landing page.

Do not use the common visual recipe of:

- Giant gradient text
- Floating glass cards everywhere
- Purple/blue gradient blobs
- Excessive glassmorphism
- Random 3D objects
- Decorative grids everywhere
- Endless rounded rectangles
- Fake terminal text purely for decoration
- Excessive marketing copy
- Huge walls of text
- Every section entering with a different animation
- Artificial "futuristic" visuals
- Generic developer illustrations
- Stock photography
- AI-generated abstract artwork

The website should instead feel like:

> **A premium Windows developer utility made by someone who actually uses Windows.**

The visual personality should combine:

- Windows 11 familiarity
- Developer tooling precision
- Modern editorial design
- Native system UI references
- Strong typography
- Restrained monochrome surfaces
- Small amounts of accent color
- Excellent interaction design
- Real product UI

Microsoft's current Windows design principles emphasize calm, familiar, effortless and coherent experiences, with color, geometry, typography and motion used intentionally rather than decoratively. PortPeek should take inspiration from those principles without pretending to be an official Microsoft product. citeturn0search11

---

# 4. Brand Personality

PortPeek should feel:

- Quiet
- Precise
- Fast
- Native
- Technical
- Confident
- Minimal
- Slightly playful
- Developer-oriented
- Practical

It should NOT feel:

- Corporate
- Enterprise
- Startup-bro
- Cyberpunk
- Hacker cosplay
- Overly futuristic
- Childish
- "AI SaaS"

The product is small.

The website should have the same confidence.

---

# 5. Visual Direction

## 5.1 Overall aesthetic

Use a dark-first interface.

The default website theme is dark.

Light mode must also exist and should follow the same design language rather than simply inverting colors.

The website should use mostly opaque surfaces.

Avoid excessive transparency.

Avoid excessive blur.

Avoid glassmorphism as the primary visual language.

Subtle translucency may be used only where it naturally resembles Windows UI.

---

# 6. Color System

The exact final values may be tuned during implementation, but the system should be based on this structure.

## Dark

```text
Background:
#080808

Background elevated:
#0D0D0D

Surface:
#111111

Surface elevated:
#161616

Surface interactive:
#1B1B1B

Border:
#242424

Border subtle:
#1A1A1A

Primary text:
#F5F5F5

Secondary text:
#A3A3A3

Muted text:
#666666

Disabled:
#454545
```

## Light

```text
Background:
#F7F7F7

Surface:
#FFFFFF

Surface elevated:
#F1F1F1

Border:
#E2E2E2

Primary text:
#171717

Secondary text:
#666666

Muted text:
#8A8A8A
```

## Accent

PortPeek should use a single restrained accent.

The final accent must be derived from the actual PortPeek brand rather than introducing a random website-only color.

Accent usage should be sparse:

- CTA
- Active navigation
- Important status
- Focus states
- Product highlights
- Small indicators
- Interactive simulator elements

Do not paint entire sections in the accent color.

---

# 7. Typography

Use **Segoe UI Variable** as the primary system-facing typeface where available.

Fallback:

```css
font-family:
"Segoe UI Variable",
"Segoe UI",
system-ui,
sans-serif;
```

Microsoft's current Windows guidance recommends Segoe UI Variable for Windows UI because it is designed for readability across sizes and densities. citeturn0search1

Do not use a fashionable Google font simply because it is fashionable.

The website should feel Windows-native first.

Typography should provide hierarchy rather than decoration.

---

# 8. Typography Rules

Use a clear hierarchy.

Example:

```text
Display:
64–88px

Hero heading:
52–72px

Section heading:
36–48px

Subheading:
20–24px

Body:
16–18px

Small body:
14px

Metadata:
12–13px
```

These are starting ranges, not absolute requirements.

Typography must scale responsively.

Avoid enormous headlines that occupy half the viewport merely to look impressive.

---

# 9. Writing Style

PortPeek copy should be:

- Short
- Direct
- Human
- Specific
- Useful
- Slightly conversational

Do not write:

> PortPeek is a revolutionary next-generation developer productivity platform designed to fundamentally transform the way modern engineers interact with localhost environments.

Write:

> **Your localhost ports. One click away.**

Then explain what it actually does.

Microsoft's writing guidance similarly emphasizes concise, clear, action-oriented language and leading with the important information. citeturn0search2

---

# 10. Visual Hierarchy

Every section must answer one question.

Examples:

```text
Hero:
What is this?

Problem:
Why do I need it?

Product:
What does it actually look like?

How it works:
How does it solve the problem?

Native:
Why is it better than another utility?

Performance:
How small is it?

Features:
What else can it do?

Download:
Where do I get it?
```

Do not create sections simply because modern landing pages usually have them.

---

# 11. Layout System

Use a consistent centered content grid.

Desktop:

```text
max-width: 1200–1280px
```

Large visual demonstrations may extend beyond the normal content width.

Text should generally remain narrower than the full grid.

Use generous whitespace.

The website should feel spacious without feeling empty.

Avoid placing every component inside a card.

Cards should be used only when grouping information is genuinely useful.

---

# 12. Borders and Geometry

Use subtle borders.

Prefer:

```text
1px solid
```

with low-contrast colors.

Border radius should be moderate.

Do not make every element extremely rounded.

Windows 11 uses approachable geometry, but PortPeek should retain a sharper developer-tool character.

Suggested radius system:

```text
4px
6px
8px
12px
16px
```

Avoid using 24–32px radius everywhere.

---

# 13. Icons

Use icons intentionally.

Prefer:

- Simple line icons
- Windows-compatible icon language
- PortPeek's actual icon
- Familiar symbols

Do not use random icon libraries for every concept if a simple CSS/icon treatment is sufficient.

Never use emoji as primary interface icons.

---

# 14. PortPeek Logo

The supplied PortPeek logo is the source of truth.

Do not redesign it.

Do not generate alternative logos.

Do not recolor it arbitrarily.

Use the provided assets consistently throughout the website.

The logo should appear naturally:

- Navbar
- Favicon
- Social preview
- Simulator
- Download section
- Footer
- Browser metadata
- Appropriate documentation surfaces

Do not plaster the logo into every section.

---

# 15. Motion System

The website should be highly interactive.

However:

> **Every meaningful animation should communicate state, hierarchy, causality or interaction.**

Good:

```text
Terminal command
↓
Port discovered
↓
Process identified
↓
PortPeek opens it
```

Good:

```text
Click tray
↓
menu appears
↓
select port
↓
localhost opens
```

Bad:

```text
Card floats forever
```

Bad:

```text
Gradient constantly moves
```

Bad:

```text
Every heading slides in from a different direction
```

Motion should feel:

- Fast
- Direct
- Native
- Responsive
- Slightly springy where appropriate
- Never theatrical

Windows design guidance describes motion as reactive, direct and context appropriate. Follow that philosophy. citeturn0search11

---

# 16. Landing Page Architecture

The landing page should contain only the strongest product story.

Required order:

```text
1. Navigation
2. Hero
3. Product preview
4. Problem → Solution
5. How PortPeek works
6. Core features
7. Lightweight/native proof
8. Developer scenarios
9. Open source/privacy
10. Download CTA
11. Footer
```

Do NOT put the full:

- Port encyclopedia
- Knowledge hub
- Blog
- Conflict solver
- Technical documentation

on the landing page.

Those have dedicated pages.

---

# 17. Navigation

Desktop navigation:

```text
PortPeek logo

Product
Simulator
Ports
Knowledge
Blog

                    GitHub
                    Download
```

Do not overcrowd the navigation.

Potential future ecosystem navigation must be architecturally possible, but do not expose imaginary products yet.

The Download button should be the visually strongest navigation action.

---

# 18. Hero

The hero must be extremely clear.

Primary message:

> **Your localhost ports. One click away.**

Supporting copy should explain:

> See what's running on your machine, identify the process behind it, and open local development servers instantly from the Windows tray.

Primary CTA:

```text
Download for Windows
```

Secondary action:

```text
Try the Simulator
```

The primary CTA must visually dominate.

---

# 19. Hero Product Visualization

The hero should NOT use a generic illustration.

Show the actual PortPeek interface.

The ideal hero visualization:

```text
Windows desktop
        ↓
Taskbar
        ↓
PortPeek tray icon
        ↓
PortPeek native menu
```

The menu should show realistic data:

```text
3000     Next.js
5173     Vite
8000     FastAPI
11434    Ollama
```

The actual PortPeek visual language must be used.

This visualization is a miniature product demonstration.

---

# 20. Hero Interaction

The hero preview may be interactive.

Example:

1. User sees tray icon.
2. PortPeek menu is closed.
3. User clicks tray icon.
4. Native-looking menu opens.
5. Ports appear.
6. User clicks a port.
7. Browser-like destination preview appears.
8. The system explains that PortPeek opened localhost.

Do not make the animation automatic so frequently that it becomes annoying.

Provide a manual interaction.

---

# 21. Product Facts

Display only verified facts.

Current target values:

```text
~500 KB
Standalone

Native Win32
Windows

<8 MB
RAM target

~0%
Idle CPU target

MIT
Open Source
```

Do not present a benchmark as a guaranteed universal value.

If a value changes in the actual application, update the website.

The website must never claim outdated metrics.

---

# 22. Problem → Solution

This section should be highly visual.

### Before PortPeek

```text
netstat -ano | findstr :3000

↓
find PID

↓
tasklist | findstr <PID>

↓
figure out what is running

↓
type localhost:3000

↓
finally open it
```

### With PortPeek

```text
Click PortPeek
      ↓
3000 · node.exe
      ↓
Click
      ↓
localhost:3000
```

The transition between these two workflows should be animated.

Do not explain the entire comparison in paragraphs.

---

# 23. How PortPeek Works

Use a visual pipeline:

```text
Windows
  ↓
TCP listening table
  ↓
Port
  ↓
PID
  ↓
Process
  ↓
Framework / service
  ↓
localhost URL
```

Each step can be interactive.

Clicking `PID` could highlight the relationship between the port and process.

The purpose is to make the technology understandable without requiring the visitor to read a technical article.

---

# 24. Native Architecture Preview

Show:

```text
Win32
C++17
Native APIs
No Electron
No WebView
Local only
```

Then visually explain:

> PortPeek talks to Windows directly instead of wrapping a browser inside a desktop app.

Where technically accurate, explain the use of native TCP table APIs such as `GetExtendedTcpTable`.

Do not exaggerate implementation details.

---

# 25. Core Features

Use a small number of strong features.

Potential cards/visual modules:

### Local port discovery

See active development ports instantly.

### Process detection

Know which process owns the port.

### Framework detection

Recognize common development environments where supported.

### One-click localhost

Open the service directly.

### Process control

If supported by the actual application, stop a stuck process from PortPeek.

### Global hotkey

Show the configured global hotkey only if implemented and verified.

Do not advertise features that aren't present in the current release.

---

# 26. Framework Detection Showcase

Use real-looking PortPeek entries:

```text
3000
node.exe
Next.js

5173
node.exe
Vite

8000
python.exe
FastAPI

8080
java.exe
Spring Boot

11434
ollama.exe
Ollama
```

These are examples, not hardcoded claims that every framework always uses that port.

The website must clearly communicate:

> "Commonly used"

rather than:

> "This port belongs to this framework."

---

# 27. Developer Scenarios

Show 3–4 scenarios.

### Frontend

```text
Next.js      3000
Vite         5173
Storybook    6006
```

### Backend

```text
FastAPI      8000
Express      3000
Spring       8080
```

### Databases

```text
PostgreSQL   5432
Redis        6379
MongoDB      27017
```

### Local AI

```text
Ollama       11434
Local API    8000
Web UI       3000
```

These are educational examples, not claims that every environment uses exactly those ports.

---

# 28. Lightweight Proof

Create a visually striking section:

```text
~500 KB
```

Then show:

```text
No Electron
No WebView
No Cloud
No Account
No Telemetry
No Background Server
```

Next to:

```text
Win32
C++17
Native Windows APIs
```

This should be one of the strongest sections of the page.

---

# 29. Open Source / Privacy

Explain:

```text
LOCAL FIRST

Your ports stay on your machine.

No account.
No cloud.
No telemetry.

Inspect the source.
Build it yourself.
```

Link to GitHub.

Only make claims that are true of the actual application.

---

# 30. Final CTA

The final CTA should be simple.

Example:

> **Stop typing localhost:3000.**

Supporting line:

> Download PortPeek and get your local development servers one click away.

Button:

```text
Download for Windows
```

Secondary:

```text
View on GitHub
```

Do not add five competing buttons.

---

# 31. Footer

Minimal footer:

```text
PortPeek

Stop typing localhost:3000.
Just click and go.

GitHub · Releases · Simulator · Ports · Knowledge · Blog

MIT License · Windows

© 2026
```

Include the PortPeek icon.

Do not create a giant corporate footer.

---

# 32. Landing Page Length

The landing page should be short enough to understand in roughly 1–2 minutes.

It should feel visually rich without being content-heavy.

The visitor should be able to stop after the hero and understand the product.

Scrolling deeper should reward curiosity rather than being required to understand the product.

---

# 33. Landing Page Rule

The landing page is **not documentation**.

It is:

> **A product experience.**

Detailed information belongs elsewhere.

---

# 34. Responsive Design

The site must work on:

- Desktop
- Laptop
- Tablet
- Mobile

The desktop experience is the primary target because PortPeek itself is a Windows desktop utility.

Do not simply collapse desktop layouts into a narrow column.

Redesign interactions where required.

The simulator must remain usable on smaller screens.

---

# 35. Accessibility

Required:

- Keyboard navigation
- Visible focus states
- Semantic HTML
- Proper button/link semantics
- Sufficient contrast
- Reduced-motion support
- Alt text for meaningful images
- No information conveyed by color alone

If the user prefers reduced motion, disable decorative and non-essential animations.

# 36. Website Information Architecture

The website is divided into focused experiences.

```text
/
├── Product landing page
│
├── /simulator
│   └── Full PortPeek interactive simulator
│
├── /ports
│   ├── Port directory
│   └── Individual port pages
│
├── /conflicts
│   └── Interactive port conflict solver
│
├── /knowledge
│   ├── Networking fundamentals
│   ├── Localhost
│   ├── TCP
│   ├── UDP
│   ├── Windows networking
│   ├── Development servers
│   ├── Databases
│   ├── AI / LLM servers
│   └── Troubleshooting
│
├── /blog
│   └── Product and engineering articles
│
└── /inside
    └── PortPeek engineering / architecture
```

---

# 37. Simulator Page

The simulator deserves a dedicated page.

It must feel like an actual miniature Windows environment.

This is not a screenshot.

This is not a card pretending to be Windows.

This is an interactive recreation of the **actual PortPeek interaction**.

---

# 38. Simulator Goal

The visitor should think:

> "Oh, this is literally how the app works."

The simulator should communicate:

```text
Click tray
↓
See ports
↓
Understand process
↓
Open localhost
```

---

# 39. Simulator Visual Environment

Create a restrained Windows 11 desktop simulation.

It should contain only enough of the desktop to make the PortPeek interaction believable.

Required:

- Desktop background
- Minimal taskbar
- System tray area
- PortPeek icon
- Realistic taskbar geometry
- Windows-like clock/system area
- PortPeek menu

Do NOT recreate the entire Windows operating system.

Do not build fake Start Menu, File Explorer, Settings, etc.

Only recreate the environment necessary for the PortPeek demonstration.

---

# 40. Simulator PortPeek UI

The PortPeek menu must closely match the actual application.

Use:

- Actual PortPeek logo
- Actual icon
- Actual spacing
- Actual typography
- Actual hierarchy
- Actual status indicators
- Actual port naming conventions

If the application UI changes, the simulator must be updated.

The simulator is not allowed to become a separate visual interpretation of PortPeek.

---

# 41. Simulator Interaction

Default state:

```text
Tray icon visible
Menu closed
```

User clicks PortPeek.

Menu opens.

Example:

```text
PortPeek
────────────────
3000   Next.js
5173   Vite
8000   FastAPI
11434  Ollama
────────────────
Open All Active
Stop Process
```

Only show actions that exist in the actual PortPeek build.

---

# 42. Simulator Scenarios

Provide multiple scenarios.

### Scenario A — Normal development

```text
3000 Next.js
5173 Vite
```

### Scenario B — Backend

```text
8000 FastAPI
8080 Spring
```

### Scenario C — Local AI

```text
11434 Ollama
3000 Web UI
```

### Scenario D — Port conflict

A process occupies a port.

PortPeek identifies it.

The user can inspect/stop it if that capability exists.

---

# 43. Simulator Guided Mode

The simulator may have a subtle guide:

```text
Click the PortPeek icon
```

After interaction:

```text
Now click a port.
```

After selecting:

```text
That's it.
PortPeek opens localhost for you.
```

The guidance should disappear once the user understands the interaction.

---

# 44. Simulator Realism

The simulator should include realistic micro-details:

- Windows-like taskbar spacing
- Correct icon sizing
- Subtle shadows
- Appropriate menu geometry
- Native-looking typography
- Light/dark theme support
- Correct hover states
- Keyboard focus
- Pointer behavior

Do not overdo blur or transparency.

The simulator should feel like a real UI, not a Dribbble concept.

---

# 45. Simulator Mobile Behavior

On mobile, do not attempt to reproduce an entire desktop.

Instead:

```text
PortPeek Simulator

[ Desktop preview ]

Tap the tray icon
```

The desktop simulation scales down but remains interactive.

---

# 46. Ports Directory

The `/ports` page is a major part of the long-term website.

It should not be limited to 35 ports.

The system should be designed to support hundreds or eventually thousands of documented ports.

The initial curated set can prioritize ports relevant to developers.

---

# 47. Port Data Model

Each port entry should contain, where information is available:

```text
Port number
Protocol
IANA service name
Common developer usage
Official service/application
Common frameworks
Typical localhost URL
Description
Port range
Security notes
Common conflicts
Common errors
Windows discovery instructions
Linux/macOS notes where useful
Related ports
Related knowledge articles
Official documentation
```

Do not claim that a port belongs exclusively to a particular framework.

---

# 48. Port Categories

Suggested categories:

```text
Web / HTTP
Frontend
Backend
JavaScript
Python
Java
Databases
Caching
AI / LLM
Containers
DevOps
Mobile Development
Game Development
Messaging
Monitoring
Infrastructure
Operating Systems
Network Services
Other
```

A port may belong to multiple categories.

---

# 49. Port Directory UI

Top:

```text
Ports

Search  [ Search ports, services, frameworks... ]

All
Frontend
Backend
Database
AI
Infrastructure
```

Below:

```text
3000
Next.js · Node · Development
TCP

5173
Vite · Frontend Development
TCP

5432
PostgreSQL
TCP
```

Keep the cards information-rich but visually restrained.

---

# 50. Port Search

Search should support:

```text
3000
Next
Next.js
node
frontend
database
Postgres
postgresql
```

Results should identify why they matched.

---

# 51. Individual Port Page

Example:

`/ports/3000`

Page structure:

```text
PORT 3000

Common development port

3000 / TCP
```

Then:

```text
Commonly associated with

Next.js
React / Node development
Express
Other JavaScript dev servers
```

Then:

```text
What is this port?
```

Then:

```text
Why do developers use it?
```

Then:

```text
What happens when you open localhost:3000?
```

Then:

```text
Common conflicts
```

Then:

```text
How to find what's using it
```

Then:

```text
How PortPeek handles it
```

Then:

```text
Related ports
```

Then:

```text
Sources
```

---

# 52. Educational Visualization

Every important networking concept should use visual explanation when possible.

For example:

```text
Browser
   │
   │ HTTP request
   ▼
localhost
127.0.0.1
   │
   │ TCP
   │
   ▼
Port 3000
   │
   ▼
Process
node.exe
   │
   ▼
Application
```

Another:

```text
IP address
127.0.0.1

      +

Port
3000

      ↓

Socket endpoint
127.0.0.1:3000
```

The purpose is to teach, not merely decorate.

---

# 53. Knowledge Hub

`/knowledge` is not a blog.

It is a structured learning resource.

The quality target should be:

> **Someone should be able to learn how ports and localhost actually work from this site.**

---

# 54. Knowledge Categories

## Fundamentals

- What is a network port?
- What is an IP address?
- What is localhost?
- What is `127.0.0.1`?
- What is `::1`?
- What is a socket?
- TCP vs UDP
- What does listening mean?
- What does binding mean?
- What is an ephemeral port?
- Port ranges
- System ports
- User ports
- Dynamic/private ports

IANA's current registry documents the standard ranges as System Ports `0–1023`, User Ports `1024–49151`, and Dynamic/Private Ports `49152–65535`. Use IANA/RFC material as the authoritative foundation for these explanations. citeturn0search4

---

# 55. Localhost Section

Explain visually:

```text
localhost
127.0.0.1
::1
```

Cover:

- What localhost means
- Loopback
- IPv4 loopback
- IPv6 loopback
- Why localhost does not mean "the internet"
- Hostname resolution
- Browser behavior
- Local development servers

---

# 56. TCP Section

Explain:

- TCP connection
- Listening socket
- Port
- Source port
- Destination port
- Connection lifecycle
- `LISTEN`
- `ESTABLISHED`
- `TIME_WAIT`
- Why a port can appear occupied
- Binding to addresses
- `0.0.0.0`
- `127.0.0.1`
- IPv6 equivalents

Use diagrams wherever possible.

---

# 57. UDP Section

Explain:

- How UDP differs from TCP
- Why UDP does not behave like a TCP listening socket
- Datagram model
- Common development uses
- Discovery
- Game networking
- DNS
- Local services

---

# 58. Windows Networking Section

Explain Windows-specific concepts:

- TCP tables
- Process IDs
- Process ownership
- `netstat`
- PowerShell networking commands
- Windows Defender Firewall
- Loopback
- Ports
- Services
- Process termination

Use Microsoft documentation as the primary Windows-specific reference.

---

# 59. Developer Server Section

Explain how development servers work.

Examples:

```text
Node
Python
Java
Go
Rust
.NET
PHP
```

Explain:

```text
Application starts
↓
Server binds address
↓
Server listens on port
↓
Browser connects
↓
Application handles request
```

---

# 60. Framework Knowledge

For frameworks, distinguish:

> **Default/common port**

from:

> **Required port**

Framework documentation should be used as the source.

Examples:

- Next.js
- Vite
- React tooling
- Angular
- Vue
- Svelte
- Astro
- Express
- FastAPI
- Flask
- Django
- Spring Boot
- ASP.NET
- Rails
- Laravel
- Go development servers
- Rust development servers

Do not assume every project uses its framework's default.

---

# 61. Database Knowledge

Document commonly encountered services such as:

- PostgreSQL
- MySQL
- MariaDB
- MongoDB
- Redis
- Elasticsearch
- ClickHouse
- SQL Server

Explain:

- What the service is
- Common port
- TCP/UDP where relevant
- Local development use
- Connection URL
- Common conflicts
- Security considerations

---

# 62. AI / Local LLM Knowledge

Create a dedicated category.

Examples:

- Ollama
- Local model servers
- OpenAI-compatible local servers
- LM Studio
- Local APIs
- Inference servers

Explain:

```text
Browser / Client
       ↓
localhost
       ↓
API port
       ↓
Model server
       ↓
GPU / CPU
       ↓
Model
```

Only document actual default ports after verifying them from official documentation.

---

# 63. Docker / Containers

Explain:

```text
Host
 ↓
Docker
 ↓
Container
 ↓
Container port
 ↓
Published host port
```

This is particularly important because beginners frequently confuse:

```text
container port
```

with:

```text
host port
```

Create visual examples.

---

# 64. Troubleshooting Knowledge

Core articles:

```text
Why is port 3000 already in use?
What does EADDRINUSE mean?
How do I find what is using a port on Windows?
How do I kill a process using a port?
Why can't I connect to localhost?
What does 0.0.0.0 mean?
Why does localhost work in one browser but not another?
Why does Docker say a port is already allocated?
Why can't two applications use the same port?
```

---

# 65. Conflict Solver

`/conflicts` should be an interactive tool.

User can enter:

```text
3000
```

or paste:

```text
EADDRINUSE: address already in use :::3000
```

The tool identifies:

```text
Likely problem:
Port 3000 is already occupied.
```

Then:

```text
What you can do

1. Find the process
2. Stop it
3. Use another port
4. Open it with PortPeek
```

---

# 66. Conflict Solver Visual

Show:

```text
Your application
      │
      │ wants
      ▼
    :3000
      │
      X
      │
Another process
already owns :3000
```

Then:

```text
PortPeek
      ↓
Find process
      ↓
node.exe
PID 18472
```

This should be interactive.

---

# 67. Conflict Solver Learning Mode

After solving the problem, explain:

- What the error means
- Why it happened
- What a PID is
- Why two applications cannot bind the same endpoint in the same way
- How Windows reports it
- How PortPeek helps

This connects the tool with the knowledge system.

---

# 68. Blog

`/blog` is editorial.

It should contain:

- PortPeek releases
- Development stories
- Windows development articles
- Technical deep dives
- Performance work
- Design decisions
- Open-source updates

Examples:

```text
Why PortPeek is written in native Win32

How we got PortPeek under 500 KB

Building a Windows tray application without Electron

How PortPeek finds the process behind a port

What I learned building a localhost utility
```

---

# 69. Knowledge vs Blog

Never mix them.

### Knowledge

Evergreen.

```text
What is localhost?
What is TCP?
What is port 3000?
```

### Blog

Time-based/editorial.

```text
PortPeek 0.5.0 is out
Why we changed the tray UI
Building PortPeek's framework detector
```

---

# 70. Inside PortPeek

Create `/inside`.

This is the technical deep-dive page.

Topics:

```text
Native Win32
C++17
TCP table discovery
Process identification
Framework detection
Loopback probing
DPI awareness
Theme synchronization
Tray integration
Global hotkey
Resource usage
Privacy
```

Use diagrams.

This page should satisfy technically curious developers.

---

# 71. Research Requirements

The knowledge system must NOT be generated from generic AI knowledge alone.

Research sources should be prioritized:

1. Official specifications
2. IANA
3. IETF RFCs
4. Microsoft Learn
5. Official framework documentation
6. Official database documentation
7. Official Docker documentation
8. Official project documentation
9. Reputable technical books
10. High-quality secondary references

IANA's registry should be the baseline for port assignments, while official project documentation should determine how a specific development tool commonly uses a port. citeturn0search4

Do not silently invent facts.

---

# 72. Source Attribution

Every knowledge article should have a small:

```text
Sources
```

section.

Example:

```text
Sources

IANA Service Name and Transport Protocol Port Number Registry
RFC 6335
Microsoft Learn
Next.js Documentation
```

Do not turn sources into an ugly bibliography.

Keep them readable.

---

# 73. Knowledge Accuracy Rules

Never write:

> "Port 3000 is the Next.js port."

Prefer:

> "Port 3000 is commonly used by JavaScript development servers, including Next.js in common development setups."

When a port is officially assigned, distinguish that from community usage.

A port's registered service name does not automatically mean every application using that port is that service.

---

# 74. SEO

The website should be optimized naturally.

Important search targets include:

```text
localhost port
port 3000
port 5173
what is port 3000
how to find process using port windows
kill process using port windows
EADDRINUSE windows
localhost already in use
find port process windows
windows port checker
localhost port finder
```

Do not keyword-stuff.

Educational quality comes first.

---

# 75. Structured Metadata

Implement:

- Proper `<title>`
- Meta description
- Open Graph metadata
- Twitter/X metadata
- Canonical URLs
- Favicon
- Web manifest if appropriate
- Structured data where appropriate
- Sitemap
- Robots metadata

Each port article should have a unique title and description.

---

# 76. Performance

The website itself should follow PortPeek's philosophy.

Avoid heavy dependencies.

Use:

- Vanilla HTML
- Vanilla CSS
- Modern JavaScript
- SVG
- Optimized images
- CSS animations
- Lazy loading
- Minimal JavaScript

Do not add React/Vue/Next.js merely to build a static GitHub Pages site.

Do not introduce a large build pipeline unless it solves a real problem.

---

# 77. GitHub Pages Architecture

The website is hosted from:

```text
/docs
```

The repository should remain easy to understand.

Suggested structure:

```text
docs/
├── index.html
├── simulator/
│   └── index.html
├── ports/
│   ├── index.html
│   ├── 3000/
│   │   └── index.html
│   ├── 5173/
│   │   └── index.html
│   └── ...
├── conflicts/
│   └── index.html
├── knowledge/
│   ├── index.html
│   └── ...
├── blog/
│   ├── index.html
│   └── ...
├── inside/
│   └── index.html
├── assets/
│   ├── images/
│   ├── icons/
│   └── fonts/
├── css/
├── js/
└── data/
```

The exact structure can be changed if a better static architecture is found, but it must remain simple and GitHub Pages compatible.

---

# 78. Content Data

Port information should ideally live in structured data rather than being manually duplicated across pages.

For example:

```json
{
  "port": 3000,
  "protocol": "TCP",
  "commonUses": [
    "Next.js",
    "Node.js development servers"
  ],
  "category": [
    "frontend",
    "javascript"
  ]
}
```

This allows:

- Directory generation
- Search
- Filtering
- Related ports
- Port pages
- Knowledge cross-links

without duplicating content.

---

# 79. Asset Rules

Use optimized assets.

Prefer:

- SVG for logos/icons where possible
- WebP/AVIF for photographs/large raster imagery
- PNG only when transparency or exact raster fidelity is required

Do not ship huge images.

Do not use background videos unless absolutely necessary.

The website should remain fast even on a slower connection.

---

# 80. Design Anti-Patterns

The implementation must NOT produce:

```text
❌ Generic SaaS hero
❌ Purple gradient
❌ Giant glass cards
❌ Random 3D illustration
❌ Stock laptop image
❌ Fake code blocks everywhere
❌ Excessive pill buttons
❌ Huge rounded cards
❌ 20 feature cards
❌ Paragraph-heavy sections
❌ Fake statistics
❌ Random decorative blobs
❌ Excessive blur
❌ Unnecessary shadows
❌ Every section animated differently
❌ Mobile layout treated as an afterthought
```

---

# 81. Visual Content Rule

When something can be explained visually, prefer the visual.

Instead of:

> PortPeek checks the Windows TCP table, obtains the process identifier and maps it to the executable.

Show:

```text
TCP TABLE
   ↓
:3000
   ↓
PID 18472
   ↓
node.exe
   ↓
Next.js
```

Then add a short explanation.

---

# 82. Content Density Rule

Do not solve a weak design by adding more copy.

If a section feels empty:

1. Improve composition.
2. Improve hierarchy.
3. Add a real product visualization.
4. Add meaningful interaction.
5. Add a useful diagram.

Only then consider adding text.

---

# 83. Final Website Personality

If someone opens the site, the reaction should be:

> "This looks like a real product."

Then:

> "Oh, it's for Windows."

Then:

> "Wait, this is tiny."

Then:

> "Oh, I can actually use this."

Then:

> "And they have a whole knowledge base about ports?"

That is the desired progression.

---

# 84. Definition of Success

The redesign succeeds when:

- The product is understandable immediately.
- The landing page is visually strong without being verbose.
- The real PortPeek UI is the star.
- The simulator feels authentic.
- The site feels Windows-native without impersonating Windows.
- Dark mode feels excellent.
- Light mode feels equally intentional.
- Animation feels purposeful.
- The knowledge section feels like a real reference resource.
- Port pages teach rather than merely list.
- Conflict Solver is genuinely interactive.
- Blog and Knowledge are clearly separated.
- The site loads quickly.
- The source remains simple.
- The website does not feel AI-generated.

---

# 85. Research Baseline

The redesign should follow current Windows design guidance for hierarchy, typography, motion, layout, usability and navigation, while maintaining PortPeek's independent identity. Microsoft currently recommends Segoe UI Variable for Windows UI typography. citeturn0search0turn0search1

The port knowledge system should use IANA/RFC material as the authoritative baseline for port assignments and ranges, with official project documentation used for framework/application usage. citeturn0search4

Do not treat these sources as the only sources; they establish the research hierarchy.

# 86. Final Implementation Requirements

The existing `/docs` website should be treated as a **failed visual implementation**, not as a design that must be preserved.

Useful content and working functionality may be retained.

The existing visual layout, styling and component decisions should NOT constrain the redesign.

The implementation should be rebuilt according to this specification.

---

# 87. What Must Be Preserved

Preserve where applicable:

- PortPeek product information
- Download functionality
- GitHub links
- Repository links
- Release links
- Simulator concept
- Port data
- Conflict solver concept
- Knowledge content
- Blog content
- Existing working utilities
- Correct technical information

Do not preserve bad UI simply because it already exists.

---

# 88. What Must Be Rebuilt

Rebuild:

- Global layout
- Navigation
- Hero
- Typography
- Colors
- Component styling
- Cards
- Buttons
- Simulator
- Animations
- Port directory
- Knowledge UI
- Conflict Solver UI
- Footer
- Responsive layouts
- Page hierarchy

---

# 89. No Feature Creep During Redesign

Do not introduce unrelated product features.

The website may become larger in content, but the product itself should not be redefined.

The purpose of the website is to present PortPeek and educate users around the domain.

---

# 90. Actual Product UI Is the Source of Truth

If screenshots, assets, source code or current builds of PortPeek are available locally, inspect them.

Do not invent a new PortPeek interface for the website.

The simulator and product visualizations must reflect the actual application.

If the actual application changes during development, update the simulator.

---

# 91. Design Review Checklist

Before considering the redesign complete, review every page and ask:

### Does it look like an AI-generated SaaS website?

If yes:

**Remove things.**

### Is there too much text?

If yes:

**Visualize the concept.**

### Are there too many cards?

If yes:

**Simplify the layout.**

### Is there too much decoration?

If yes:

**Remove it.**

### Does it feel like Windows?

If no:

**Improve typography, geometry, spacing and interaction patterns.**

### Does it feel like PortPeek?

If no:

**Use the real product UI and brand assets.**

---

# 92. Final Experience

The complete site should feel like:

```text
                 PORTPEEK
                    │
          ┌─────────┴─────────┐
          │                   │
       PRODUCT             KNOWLEDGE
          │                   │
      Download           Understand
          │                   │
       Simulator          Explore ports
          │                   │
       Try it             Fix problems
```

The website is simultaneously:

> **A product website**

and

> **A developer reference resource.**

The landing page sells.

The rest teaches.

---

# 93. FINAL /goal PROMPT

The following prompt should be used as the implementation instruction for the coding agent.

---

## `/goal`

Rebuild the **entire PortPeek GitHub Pages website** from the ground up.

The current website inside `/docs` is considered a failed visual implementation.

Do not attempt to cosmetically patch the current UI.

Keep useful content and functionality where appropriate, but redesign the entire experience according to this specification.

---

## Product

PortPeek is a tiny native Windows developer utility.

Its purpose is:

> **See what's running on localhost and open it instantly.**

Current verified project positioning:

- Native Windows utility
- Win32 / C++17
- Approximately **500 KB** standalone executable target
- Very low idle resource usage
- No Electron
- No WebView
- Local-first
- Open source
- Windows-focused
- System-tray based

Do NOT use the outdated `~340 KB` number anywhere.

---

# Design Direction

The website must feel:

- Premium
- Minimal
- Native Windows-inspired
- Developer-focused
- Technical but approachable
- Calm
- Fast
- Precise
- Highly polished

It must NOT feel like:

- Generic SaaS
- AI-generated website
- Web3 website
- Cyberpunk website
- Glassmorphism showcase
- Startup template
- Corporate enterprise software
- Random Dribbble concept

Do not use excessive gradients.

Do not use excessive glass.

Do not use decorative blobs.

Do not fill the site with rounded cards.

Do not compensate for weak visual design by adding huge amounts of copy.

---

# Windows 11 Inspiration

Use Windows 11 as a design reference, not as something to imitate literally.

The experience should be:

> **Windows-native in spirit, PortPeek-native in identity.**

Use:

- Clear hierarchy
- Familiar geometry
- System-oriented typography
- Restrained surfaces
- Subtle elevation
- Purposeful motion
- Strong spacing
- Familiar interaction patterns

Use **Segoe UI Variable** as the preferred primary font.

Support dark and light themes.

Dark is the default.

---

# Theme

Dark:

```text
#080808
#0D0D0D
#111111
#161616
#1B1B1B
#242424

#F5F5F5
#A3A3A3
#666666
```

Light:

```text
#F7F7F7
#FFFFFF
#F1F1F1
#E2E2E2

#171717
#666666
#8A8A8A
```

Use one restrained PortPeek accent.

Do not introduce multiple bright accent colors.

---

# Typography

Use:

```text
Segoe UI Variable
Segoe UI
system-ui
sans-serif
```

Typography should create hierarchy.

Avoid giant text for the sake of visual drama.

Use short, direct copy.

Never write generic AI-generated marketing paragraphs.

---

# Navigation

Desktop:

```text
PortPeek

Product
Simulator
Ports
Knowledge
Blog

GitHub
Download
```

Download must be the strongest CTA.

Keep navigation minimal.

---

# Landing Page

The landing page must be short.

Structure:

```text
Navigation
↓
Hero
↓
Real PortPeek UI
↓
Problem → Solution
↓
How it works
↓
Features
↓
Native / lightweight proof
↓
Developer scenarios
↓
Privacy / Open Source
↓
Download
↓
Footer
```

Do NOT put the complete knowledge base on the landing page.

Do NOT put the complete port directory on the landing page.

Do NOT put the full conflict solver on the landing page.

---

# Hero

Use:

> **Your localhost ports. One click away.**

Supporting text should be concise.

Primary:

```text
Download for Windows
```

Secondary:

```text
Try the Simulator
```

Show the actual PortPeek UI.

Do not use an abstract illustration.

---

# Product Visualization

Create a highly polished representation of the actual PortPeek tray experience.

It should show:

```text
3000    Next.js
5173    Vite
8000    FastAPI
11434   Ollama
```

Only use examples that match actual supported behavior.

Use the real PortPeek icon and visual language.

---

# Animation

The site should be highly interactive.

However, animations must communicate:

- Interaction
- State
- Cause and effect
- Navigation
- Technical relationships

Examples:

```text
TCP table
↓
Port
↓
PID
↓
Process
↓
Framework
```

and:

```text
Click tray
↓
Menu
↓
Port
↓
localhost
```

Avoid decorative infinite animation.

Support `prefers-reduced-motion`.

---

# Simulator Page

Create:

```text
/simulator/
```

This must be a full interactive recreation of the PortPeek experience.

It should include:

- Windows 11-inspired desktop
- Taskbar
- System tray
- PortPeek icon
- PortPeek menu
- Realistic port entries
- Hover states
- Click behavior
- Open localhost simulation

The simulator must feel like the actual PortPeek application.

Do not make a generic mockup.

Do not make it a screenshot.

Do not recreate the entire Windows OS.

Only recreate the parts required for the PortPeek interaction.

---

# Ports Page

Create:

```text
/ports/
```

Build a searchable and filterable port directory.

It should be designed to eventually contain hundreds or thousands of entries.

Support:

- Port number
- Protocol
- Common usage
- Application/framework
- Category
- Description
- Related ports
- Troubleshooting
- Official sources

Search should support:

```text
3000
Next.js
node
frontend
database
```

---

# Individual Port Pages

Create pages such as:

```text
/ports/3000/
/ports/5173/
/ports/5432/
/ports/6379/
/ports/8000/
/ports/8080/
/ports/11434/
```

Each page should explain:

```text
What is this port?
What commonly uses it?
Why is it used?
TCP / UDP
Typical localhost URL
How it works
Common conflicts
Common errors
How to find the owning process
How to fix conflicts
How PortPeek helps
Related ports
Sources
```

Do not claim that a port belongs exclusively to a framework.

Clearly distinguish:

- Official registration
- Common developer usage
- Project defaults
- Community conventions

---

# Knowledge Hub

Create:

```text
/knowledge/
```

This should become a serious educational resource.

It must cover:

```text
Ports
Networking
TCP
UDP
Localhost
Loopback
Sockets
Windows networking
Development servers
Databases
Containers
Docker
AI / LLM servers
Troubleshooting
```

The quality target is:

> A developer should be able to genuinely learn how ports and localhost work by reading this website.

Use visual diagrams.

Use interactive explanations where useful.

Do not make it a wall of documentation.

---

# Research Requirement

Knowledge must be researched.

Do not rely exclusively on model knowledge.

Prioritize:

1. IANA
2. IETF RFCs
3. Microsoft Learn
4. Official framework documentation
5. Official database documentation
6. Official Docker documentation
7. Official project documentation
8. Well-established technical books
9. High-quality secondary references

IANA is the authoritative foundation for registered service/port assignments and port ranges.

Use official project documentation for framework/application defaults.

Never invent port assignments.

Never imply that a registered port is exclusively owned by one application.

Include concise source attribution on knowledge pages.

---

# Conflict Solver

Create:

```text
/conflicts/
```

Allow the user to enter:

```text
3000
```

or paste an error:

```text
EADDRINUSE: address already in use :::3000
```

Explain:

```text
What happened
Why it happened
How to find the process
How to stop it
How to choose another port
How PortPeek helps
```

Use visual diagrams.

Make it interactive rather than just an article.

---

# Blog

Create:

```text
/blog/
```

Keep Blog separate from Knowledge.

Blog:

- Product releases
- Development stories
- Engineering articles
- Design decisions
- Performance work
- Open-source updates

Knowledge:

- Evergreen educational material

Do not mix the two.

---

# Inside PortPeek

Create:

```text
/inside/
```

Explain:

- Win32
- C++17
- TCP table discovery
- Process identification
- Framework detection
- Loopback probing
- DPI awareness
- Windows theme support
- Tray integration
- Performance
- Privacy

Use diagrams and actual technical explanations.

Do not turn this into a generic "technology stack" section.

---

# Content Philosophy

Use:

> **Visual explanation first. Text second.**

Instead of explaining:

> PortPeek maps TCP entries to process IDs...

show:

```text
TCP table
    ↓
:3000
    ↓
PID 18472
    ↓
node.exe
    ↓
Next.js
```

Then explain it.

---

# Performance

The website must be lightweight.

Use:

- Vanilla HTML
- CSS
- JavaScript
- SVG
- Optimized assets

Avoid:

- React
- Next.js
- Electron
- Heavy UI libraries
- Large animation libraries
- Unnecessary dependencies

The site is hosted on GitHub Pages.

---

# GitHub Pages

The website lives inside:

```text
/docs
```

Maintain a simple static architecture.

Suggested:

```text
docs/
├── index.html
├── simulator/
├── ports/
├── conflicts/
├── knowledge/
├── blog/
├── inside/
├── assets/
├── css/
├── js/
└── data/
```

Use structured port data to avoid duplicating information.

---

# SEO

Implement:

- Unique page titles
- Meta descriptions
- Canonical URLs
- Open Graph
- Social preview
- Sitemap
- Robots metadata
- Structured data where appropriate

Target real searches such as:

```text
port 3000
port 5173
localhost port
EADDRINUSE windows
find process using port windows
kill process using port windows
what is localhost
what is a network port
```

Do not keyword stuff.

---

# Accessibility

Implement:

- Semantic HTML
- Keyboard navigation
- Visible focus
- Good contrast
- Reduced motion
- Accessible buttons
- Accessible links
- Meaningful alt text

---

# Responsive Design

Desktop is the primary experience.

However, every page must work on mobile.

Do not simply stack every desktop element.

The simulator needs a mobile-specific presentation.

---

# Branding

Use the provided PortPeek assets.

Do not generate replacement logos.

Do not redesign the logo.

Use the official icon for:

- Favicon
- Navbar
- Simulator
- Social metadata
- Footer
- Appropriate product UI representations

The actual product UI is the visual source of truth.

---

# MOST IMPORTANT RULE

Do not make the website look like it was generated from a generic landing-page template.

If you are unsure whether a visual element is necessary:

**remove it.**

If a section needs more explanation:

**visualize it before adding more text.**

If a card exists only because modern websites have cards:

**remove it.**

If an animation exists only because the site needs animation:

**remove it.**

If an illustration could be replaced with the actual PortPeek UI:

**use the actual PortPeek UI.**

---

# Definition of Done

The website is complete only when:

- Landing page is short and compelling.
- Download is immediately obvious.
- PortPeek's actual UI is showcased.
- Simulator feels real.
- Windows 11 influence is visible without copying Windows.
- Dark mode is excellent.
- Light mode is intentional.
- Animations are purposeful.
- No generic AI/SaaS aesthetics remain.
- Port directory is searchable.
- Individual port pages are useful.
- Knowledge hub is genuinely educational.
- Knowledge uses authoritative research.
- Conflict Solver works interactively.
- Blog is separate from Knowledge.
- Inside page explains the technology.
- Mobile works.
- Accessibility works.
- SEO is implemented.
- GitHub Pages works.
- Site remains lightweight.
- The entire experience feels like one coherent PortPeek product.

---

# Final Quality Test

Before finishing, open the website as a developer who has never seen PortPeek.

Ask:

> What is PortPeek?

The answer should be obvious within seconds.

Then ask:

> Why should I use it?

The answer should be obvious.

Then:

> What does it actually look like?

The answer should be visible.

Then:

> Why is it different?

The answer should be:

```text
Native.
Tiny.
Fast.
Local.
One click.
```

Then:

> Can I learn something here?

The answer should be:

**Yes.**

And finally:

> Can I download it?

The answer should be:

**Yes — immediately.**

Do not ship until all six answers are obvious.

---

# End of `/goal`