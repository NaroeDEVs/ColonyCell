let cells = [];
let globalIdCounter = 1;
let lastCompactOutput = "";
let lastDetailedOutput = "";
let currentViewMode = 0; 

window.isWasmReady = window.isWasmReady || false;

// DOM Selections
const navTool = document.getElementById('navTool');
const navFaq = document.getElementById('navFaq');
const navInfo = document.getElementById('navInfo');
const pageTool = document.getElementById('pageTool');
const pageFaq = document.getElementById('pageFaq');
const pageInfo = document.getElementById('pageInfo');
const themeToggle = document.getElementById('themeToggle');
const mainLogo = document.getElementById('mainLogo');
const btnCompact = document.getElementById('btnCompact');
const btnDetailed = document.getElementById('btnDetailed');
const resultBox = document.getElementById('resultBox');
const summaryBox = document.getElementById('summaryBox');
const csvFile = document.getElementById('csvFile');
const cellList = document.getElementById('cellList');

// Initialize Theme
if (localStorage.getItem('theme') === 'dark') {
    document.body.classList.add('dark-mode');
    themeToggle.innerText = 'Light Mode';
    if(mainLogo) mainLogo.src = 'ColonyCell logo light.png';
} else {
    document.body.classList.remove('dark-mode');
    themeToggle.innerText = 'Dark Mode';
    if(mainLogo) mainLogo.src = 'ColonyCell logo.png';
}

themeToggle.addEventListener('click', () => {
    document.body.classList.toggle('dark-mode');
    if (document.body.classList.contains('dark-mode')) {
        localStorage.setItem('theme', 'dark');
        themeToggle.innerText = 'Light Mode';
        if(mainLogo) mainLogo.src = 'ColonyCell logo light.png';
    } else {
        localStorage.setItem('theme', 'light');
        themeToggle.innerText = 'Dark Mode';
        if(mainLogo) mainLogo.src = 'ColonyCell logo.png';
    }
});

// Input Sanitization Filters
function enforceIntegerInput(e) {
    if (e.ctrlKey || e.metaKey || ["Backspace", "ArrowLeft", "ArrowRight", "Tab", "Delete", "Enter"].includes(e.key)) return;
    if (!/^[0-9]$/.test(e.key)) e.preventDefault();
}

function enforceFloatInput(e) {
    if (e.ctrlKey || e.metaKey || ["Backspace", "ArrowLeft", "ArrowRight", "Tab", "Delete", "Enter"].includes(e.key)) return;
    if (!/^[0-9.]$/.test(e.key)) e.preventDefault();
    if (e.key === '.' && e.target.value.includes('.')) e.preventDefault();
}

['series', 'parallel', 'wCap', 'wRes', 'cId', 'cCap'].forEach(id => {
    let el = document.getElementById(id);
    if(el) {
        el.addEventListener('keydown', enforceIntegerInput);
        el.addEventListener('input', (e) => e.target.value = e.target.value.replace(/[^0-9]/g, ''));
    }
});

['nomVolt', 'maxVolt', 'cIr'].forEach(id => {
    let el = document.getElementById(id);
    if(el) {
        el.addEventListener('keydown', enforceFloatInput);
        el.addEventListener('input', (e) => {
            e.target.value = e.target.value.replace(/[^0-9.]/g, '');
            let parts = e.target.value.split('.');
            if (parts.length > 2) e.target.value = parts[0] + '.' + parts.slice(1).join('');
        });
    }
});

// Navigation Flow
function switchTab(activeNav, activePage) {
    [navTool, navFaq, navInfo].forEach(n => n.classList.remove('active'));
    [pageTool, pageFaq, pageInfo].forEach(p => p.classList.add('hidden'));
    activeNav.classList.add('active');
    activePage.classList.remove('hidden');
}

navTool.addEventListener('click', () => switchTab(navTool, pageTool));
navFaq.addEventListener('click', () => switchTab(navFaq, pageFaq));
navInfo.addEventListener('click', () => switchTab(navInfo, pageInfo));

// Output View Toggles
if (btnCompact) {
    btnCompact.addEventListener('click', () => {
        currentViewMode = 0;
        btnCompact.classList.add('active');
        btnDetailed.classList.remove('active');
        if (lastCompactOutput && resultBox) resultBox.innerHTML = lastCompactOutput;
    });
}

if (btnDetailed) {
    btnDetailed.addEventListener('click', () => {
        currentViewMode = 1;
        btnDetailed.classList.add('active');
        btnCompact.classList.remove('active');
        if (lastDetailedOutput && resultBox) resultBox.innerHTML = lastDetailedOutput;
    });
}

// UI Updating System
function updateUI() {
    const countEl = document.getElementById('cellCount');
    if (countEl) countEl.innerText = `Total: ${cells.length}`;
    if (!cellList) return;
    
    cellList.innerHTML = '';
    [...cells].reverse().forEach((c, index) => {
        let realIndex = cells.length - 1 - index;
        let li = document.createElement('li');
        li.innerHTML = `<div class="cell-info"><b>ID: ${c.id}</b> | Capacity: ${c.cap}mAh | Resistance: ${c.ir}m&Omega; <br> <small>Manufacturer: ${c.mfg} | Condition: ${c.cond}</small></div>
                        <button class="del-btn" onclick="removeCell(${realIndex})">X</button>`;
        cellList.appendChild(li);
    });
}

window.removeCell = function(index) {
    cells.splice(index, 1);
    updateUI();
}

// Clear Form Controls
const clearAllBtn = document.getElementById('clearAllBtn');
if (clearAllBtn) {
    clearAllBtn.addEventListener('click', () => {
        cells = [];
        updateUI();
        if (summaryBox) summaryBox.innerHTML = "<div class='meter-box'><span>Status</span><strong>Waiting for input...</strong></div>";
        if (resultBox) resultBox.innerHTML = "";
        if (csvFile) csvFile.value = "";
        lastCompactOutput = "";
        lastDetailedOutput = "";
    });
}

// Form Submission (Add Cells Manually)
const addCellBtn = document.getElementById('addCellBtn');
if (addCellBtn) {
    addCellBtn.addEventListener('click', () => {
        let idInput = document.getElementById('cId').value;
        let capInput = document.getElementById('cCap').value;
        let irInput = document.getElementById('cIr').value;
        let mfg = document.getElementById('cMfg').value || "N/A";
        let cond = document.getElementById('cCond').value || "N/A";
        
        if (!capInput || capInput.trim() === "") {
            alert("Capacity is required!");
            return;
        }
        
        let id = idInput ? parseInt(idInput) : globalIdCounter++;
        let cap = parseInt(capInput);
        let ir = irInput ? parseFloat(irInput) : 0;

        cells.push({id: id, cap: cap, ir: ir, mfg: mfg, cond: cond});
        
        document.getElementById('cId').value = '';
        document.getElementById('cCap').value = '';
        document.getElementById('cIr').value = '';
        document.getElementById('cMfg').value = '';
        document.getElementById('cCond').value = '';
        updateUI();
    });
}

// Demo Loading Logic
const demoBtn = document.getElementById('demoBtn');
if (demoBtn) {
    demoBtn.addEventListener('click', () => {
        cells = [];
        document.getElementById('series').value = 14;
        document.getElementById('parallel').value = 4;
        document.getElementById('wCap').value = 80;
        document.getElementById('wRes').value = 20;

        for (let i = 1; i <= 56; i++) {
            let randCap = Math.floor(Math.random() * (2600 - 2100 + 1)) + 2100;
            let randIr = Math.floor(Math.random() * (60 - 35 + 1)) + 35;
            cells.push({
                id: i, 
                cap: randCap, 
                ir: randIr, 
                mfg: "Randomised Demo", 
                cond: "Used"
            });
        }
        updateUI();
        const runBtn = document.getElementById('runBtn');
        if (runBtn) runBtn.click();
    });
}

// CSV Management
if (csvFile) {
    csvFile.addEventListener('change', (e) => {
        if (!e.target.files[0]) return;
        let reader = new FileReader();
        reader.onload = (evt) => {
            let lines = evt.target.result.split('\n');
            for (let i = 0; i < lines.length; i++) {
                let line = lines[i].trim();
                if (line === "" || line.startsWith("ID;")) continue;
                let parts = line.split(';');
                
                if (parts.length >= 2 && parts[1].trim() !== "") {
                    let parsedCap = parseInt(parts[1].replace(/[^0-9]/g, ''));
                    if (isNaN(parsedCap)) continue;
                    
                    cells.push({
                        id: parts[0] ? parseInt(parts[0].replace(/[^0-9]/g, '')) || globalIdCounter++ : globalIdCounter++,
                        cap: parsedCap,
                        ir: parts[2] ? parseFloat(parts[2].replace(/[^0-9.]/g, '')) || 0 : 0,
                        mfg: parts[3] ? parts[3].trim() : "N/A",
                        cond: parts[4] ? parts[4].trim() : "N/A"
                    });
                }
            }
            updateUI();
        };
        reader.readAsText(e.target.files[0]);
    });
}

// Running WebAssembly Core Code Calculation
const runBtn = document.getElementById('runBtn');
if (runBtn) {
    runBtn.addEventListener('click', () => {
        if (typeof Module.OptimizePack !== "function") {
            if (summaryBox) summaryBox.innerHTML = "<div class='meter-box'><strong style='color:red'>WASM Loading... Try again in a second.</strong></div>";
            return;
        }
        
        let series = parseInt(document.getElementById('series').value) || 16;
        let parallel = parseInt(document.getElementById('parallel').value) || 5;

        let requiredCells = series * parallel;
        if (cells.length < requiredCells) {
            if (summaryBox) summaryBox.innerHTML = `<div class='meter-box full-width'><strong style='color:red; font-size: 1.1rem;'>Error: Not enough cells! You need ${requiredCells} cells for a ${series}S${parallel}P pack, but you only have ${cells.length}.</strong></div>`;
            return; 
        }

        let wCap = parseFloat(document.getElementById('wCap').value) || 80;
        let wRes = parseFloat(document.getElementById('wRes').value) || 20;
        let nomVolt = parseFloat(document.getElementById('nomVolt').value) || 3.6;
        let maxVolt = parseFloat(document.getElementById('maxVolt').value) || 4.2;

        let csvString = "ID;Capacity;Resistance;Manufacturer;Condition\n";
        cells.forEach(c => {
            csvString += `${c.id};${c.cap};${c.ir};${c.mfg};${c.cond}\n`;
        });

        if (summaryBox) summaryBox.innerHTML = "<div class='meter-box'><strong>Calculating...</strong></div>";
        
        setTimeout(() => {
            try {
                let result = Module.OptimizePack(csvString, series, parallel, wCap, wRes, nomVolt, maxVolt);
                let parts = result.split("|||");
                
                if (parts.length === 3) {
                    if (summaryBox) summaryBox.innerHTML = parts[0];
                    lastCompactOutput = parts[1];
                    lastDetailedOutput = parts[2];
                    if (resultBox) resultBox.innerHTML = currentViewMode === 1 ? lastDetailedOutput : lastCompactOutput;
                } else {
                    let errorMessage = parts.length > 1 ? parts[1] : parts[0];
                    if (summaryBox) summaryBox.innerHTML = "<div class='meter-box'><strong style='color:red'>" + errorMessage + "</strong></div>";
                    if (resultBox) resultBox.innerHTML = "";
                    lastCompactOutput = "";
                    lastDetailedOutput = "";
                }
            } catch (error) {
                if (summaryBox) summaryBox.innerHTML = "<div class='meter-box'><strong style='color:red'>Algorithm Crash</strong></div>";
            }
        }, 50);
    });
}

// Logo Reset Interactivity
const logoContainer = document.querySelector('.logo');
if (logoContainer) {
    logoContainer.addEventListener('click', () => {
        switchTab(navTool, pageTool);

        cells = [];
        updateUI();

        if (summaryBox) summaryBox.innerHTML = "<div class='meter-box'><span>Status</span><strong>Waiting for input...</strong></div>";
        if (resultBox) resultBox.innerHTML = "";
        if (csvFile) csvFile.value = "";
        lastCompactOutput = "";
        lastDetailedOutput = "";

        document.getElementById('series').value = "16";
        document.getElementById('parallel').value = "5";
        document.getElementById('wCap').value = "80";
        document.getElementById('wRes').value = "20";
        document.getElementById('nomVolt').value = "3.6";
        document.getElementById('maxVolt').value = "4.2";

        currentViewMode = 0;
        if (btnCompact) btnCompact.classList.add('active');
        if (btnDetailed) btnDetailed.classList.remove('active');
    });
}