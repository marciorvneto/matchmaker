# Matchmaker

**A fast, WebAssembly-powered structural analyzer for Equation-Oriented Architecture (EOA).**

Matchmaker is a browser-based computational tool that parses systems of equations, verifies degrees of freedom, and performs bipartite matching to assign variables to equations. Built with a custom zero-dependency C engine compiled to WebAssembly, it visualizes the highly coupled nature of thermodynamic and chemical engineering models in a force-directed "spiderman" graph.

### Background & The Bigger Picture

This project is a standalone, interactive extraction of the structural analysis phase from my doctoral research on Equation-Oriented Multiple Effect Evaporator (MEE) simulation.

You can read the full thesis here:
🔗 **[Synthesis and optimization of Kraft process evaporator plants](https://lutpub.lut.fi/handle/10024/162289)**

In a full EOA simulator, finding a perfect bipartite match is just step one. To solve the complete system of equations corresponding to the scenario being simulated, the complete pipeline looks like this:

1. **Lexical Parsing & Graph Generation:** Converting string equations into an Abstract Syntax Tree and bipartite graph. _(Matchmaker handles this)_
2. **Structural Matching:** Using Kuhn's Algorithm to verify degrees of freedom ($DoF = 0$) and ensure mathematical well-posedness. _(Matchmaker handles this)_
3. **Block Lower Triangular (BLT) Transformation:** Using Tarjan’s strongly connected components (SCC) algorithm to identify algebraic loops and order the equations for execution.
4. **Numerical Solution:** Handing the torn sub-systems off to a Newton-Raphson solver.

### Tech Stack

- **Kernel:** Pure C. Features a custom arena allocator, a hand-rolled tokenizer, and an implementation of Kuhn's algorithm for maximum bipartite matching.
- **Bridge:** Emscripten, compiling the C kernel to a microscopic WebAssembly module (`.wasm`).
- **Frontend:** Vanilla JavaScript (ES6 Modules) and HTML/CSS, styled like a professional IDE.
- **Visualization:** `force-graph` rendered on an HTML5 `<canvas>`.

### Running Locally

Because the engine is loaded as an ES6 Module, it must be run over a local server to bypass browser CORS restrictions.

1. Clone the repository.
2. Start a local Python server:

```bash
python -m http.server

```

3. Open `http://localhost:8000` in your browser.

### Powered By

This engine is a lightweight demonstration of the core technologies powering **[Voima Toolbox](https://voimatoolbox.com/en-beta)**, a next-generation diagramming and calculation platform for engineers.

### Acknowledgements

I'd like to mention that none of this would have been possible without the insightful feedback given to me by my doctoral supervisors, Esa K. Vakkilainen and Éder D. Oliveira. For all the learning and the fruitful conversations, I will forever be grateful.
