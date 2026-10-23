// TIP Gaming Shop - browser version of main.cpp
// Data is saved in this browser (localStorage), so it is not shared between visitors.

var MAX = 10;
var KEY = "tipShopData";

function defaultData() {
    return {
        items: [
            { id: 101, name: "Mech Keyboard", price: 2500, stock: 20 },
            { id: 102, name: "Gaming Mouse", price: 1200, stock: 20 },
            { id: 103, name: "1TB SSD", price: 3500, stock: 15 }
        ],
        totalTransactions: 0,
        totalRevenue: 0
    };
}

function loadData() {
    try {
        var raw = localStorage.getItem(KEY);
        if (raw) return JSON.parse(raw);
    } catch (e) {}
    return defaultData();
}

function saveData() {
    try { localStorage.setItem(KEY, JSON.stringify(data)); } catch (e) {}
}

var data = loadData();

function getFailed() {
    try { return parseInt(sessionStorage.getItem("failedAttempts") || "0", 10); } catch (e) { return 0; }
}
function setFailed(n) {
    try { sessionStorage.setItem("failedAttempts", String(n)); } catch (e) {}
}

function findIndex(id) {
    for (var i = 0; i < data.items.length; i++) {
        if (data.items[i].id === id) return i;
    }
    return -1;
}

function validRole(role) {
    return ["Supervisor", "InventoryAdmin", "Cashier", "Customer"].indexOf(role) !== -1;
}

function canDo(role, action) {
    if (action === "sale") return ["Supervisor", "Cashier", "Customer"].indexOf(role) !== -1;
    if (action === "restock" || action === "add") return ["InventoryAdmin", "Supervisor"].indexOf(role) !== -1;
    if (action === "report") return ["Supervisor", "InventoryAdmin", "Cashier"].indexOf(role) !== -1;
    return false;
}

function money(n) { return "P" + Number(n).toFixed(2); }

function safeText(s) {
    return String(s)
        .replace(/&/g, "&amp;")
        .replace(/</g, "&lt;")
        .replace(/>/g, "&gt;")
        .replace(/"/g, "&quot;");
}

// inactivity timer: 2 minutes, then back to the home page
function startTimer() {
    var timer;
    function reset() {
        clearTimeout(timer);
        timer = setTimeout(function () { window.location = "index.html?timeout=1"; }, 120000);
    }
    document.onmousemove = reset;
    document.onkeydown = reset;
    document.onclick = reset;
    reset();
}

/* ---------------- home page: login ---------------- */

function setupHome() {
    var msg = document.getElementById("loginMsg");
    if (location.search.indexOf("timeout=1") !== -1) {
        msg.textContent = "Session ended due to inactivity. Please log in again.";
    }

    document.getElementById("customerForm").onsubmit = function (e) {
        e.preventDefault();
        var name = this.username.value.trim();
        if (name === "") return;
        if (["Supervisor", "InventoryAdmin", "Cashier"].indexOf(name) !== -1) {
            msg.textContent = "That is a staff account. Use the staff login below.";
            return;
        }
        location.href = "dashboard.html?role=Customer&name=" + encodeURIComponent(name);
    };

    document.getElementById("staffForm").onsubmit = function (e) {
        e.preventDefault();
        var u = this.username.value.trim();
        var p = this.password.value;

        if (u === "") { msg.textContent = "Please enter a username."; return; }
        if (["Supervisor", "InventoryAdmin", "Cashier"].indexOf(u) === -1) {
            location.href = "dashboard.html?role=Customer&name=" + encodeURIComponent(u);
            return;
        }
        if (getFailed() >= 3) {
            msg.textContent = "3 failed password attempts. System locked, close this tab and open the site again.";
            return;
        }
        var role = "";
        if (u === "Supervisor" && p === "SupV123") role = "Supervisor";
        else if (u === "InventoryAdmin" && p === "Admin123") role = "InventoryAdmin";
        else if (u === "Cashier" && p === "Cashier123") role = "Cashier";

        if (role === "") {
            setFailed(getFailed() + 1);
            msg.textContent = "Wrong password! Attempts left: " + (3 - getFailed());
            return;
        }
        setFailed(0);
        location.href = "dashboard.html?role=" + role;
    };
}

/* ---------------- dashboard ---------------- */

var role = "";
var view;

function params() {
    var out = {};
    location.search.replace(/^\?/, "").split("&").forEach(function (pair) {
        if (!pair) return;
        var kv = pair.split("=");
        out[decodeURIComponent(kv[0])] = decodeURIComponent((kv[1] || "").replace(/\+/g, " "));
    });
    return out;
}

function messageView(title, msg, extraHtml) {
    view.innerHTML = "<h1>" + title + "</h1><section class=\"card\"><p>" + msg + "</p>" +
        (extraHtml || "") +
        "<a class=\"linkbtn\" href=\"#\" id=\"backBtn\">Back to dashboard</a></section>";
    document.getElementById("backBtn").onclick = function (e) { e.preventDefault(); renderDashboard(); };
    window.scrollTo(0, 0);
}

function renderDashboard() {
    var h = "";
    var p = params();
    h += "<div class=\"who\"><h1 style=\"margin:0\">Dashboard</h1>";
    h += "<span class=\"badge good\">" + safeText(role) + "</span>";
    h += "<a class=\"linkbtn\" href=\"index.html\">Log out</a></div>";
    if (role === "Customer" && p.name) {
        h += "<p class=\"muted\">Welcome, " + safeText(p.name) + "!</p>";
    }

    h += "<h2>Shop</h2><div class=\"shelf\">";
    data.items.forEach(function (it, i) {
        h += "<div class=\"item tone" + (i % 5) + "\">";
        h += "<div class=\"pic\"><span>" + safeText(it.name.charAt(0)) + "</span></div>";
        h += "<div class=\"info\"><h3>" + safeText(it.name) + "</h3>";
        h += "<p class=\"muted\">ID " + it.id + " | Stock " + it.stock + "</p>";
        h += "<div class=\"priceRow\"><b>" + money(it.price) + "</b>";
        if (it.stock === 0) h += "<span class=\"badge out\">Out of stock</span>";
        else if (it.stock <= 5) h += "<span class=\"badge low\">Low stock</span>";
        else h += "<span class=\"badge good\">In stock</span>";
        h += "</div>";

        if (canDo(role, "sale")) {
            if (it.stock === 0) {
                h += "<button disabled>Sold out</button>";
            } else {
                h += "<form class=\"buyform\" data-id=\"" + it.id + "\">";
                h += "<input type=\"number\" name=\"qty\" min=\"1\" value=\"1\" required>";
                h += "<input type=\"number\" name=\"cash\" min=\"0\" step=\"0.01\" placeholder=\"Cash\" required>";
                h += "<button type=\"submit\">Buy</button></form>";
            }
        }
        h += "</div></div>";
    });
    h += "</div>";

    if (canDo(role, "restock")) {
        h += "<div class=\"grid2\">";
        h += "<section class=\"card\"><h2>Restock a product</h2><form id=\"restockForm\">";
        h += "<label>Item<select name=\"id\">";
        data.items.forEach(function (it) {
            h += "<option value=\"" + it.id + "\">" + it.id + " - " + safeText(it.name) + "</option>";
        });
        h += "</select></label>";
        h += "<label>Amount to add<input type=\"number\" name=\"amount\" min=\"1\" max=\"30\" required></label>";
        h += "<button type=\"submit\">Restock</button></form></section>";

        h += "<section class=\"card\"><h2>Add a new product</h2><form id=\"addForm\">";
        h += "<label>Product ID<input type=\"number\" name=\"id\" required></label>";
        h += "<label>Product name<input type=\"text\" name=\"name\" required></label>";
        h += "<label>Price (P)<input type=\"number\" name=\"price\" min=\"0\" step=\"0.01\" required></label>";
        h += "<label>Starting stock<input type=\"number\" name=\"stock\" min=\"1\" max=\"30\" required></label>";
        h += "<button type=\"submit\">Add product</button></form></section>";
        h += "</div>";
    }

    if (canDo(role, "report")) {
        h += "<section class=\"card\"><h2>Daily sales report</h2>";
        h += "<p>Total transactions: <b>" + data.totalTransactions + "</b></p>";
        h += "<p>Total revenue: <b>" + money(data.totalRevenue) + "</b></p>";
        h += "<p>Out of stock items:</p>";
        var found = false;
        data.items.forEach(function (it) {
            if (it.stock === 0) { h += "<p>- " + safeText(it.name) + "</p>"; found = true; }
        });
        if (!found) h += "<p class=\"muted\">None. All items have stock.</p>";
        h += "</section>";
    }

    view.innerHTML = h;
    window.scrollTo(0, 0);

    Array.prototype.forEach.call(document.querySelectorAll(".buyform"), function (f) {
        f.onsubmit = function (e) {
            e.preventDefault();
            processSale(parseInt(f.getAttribute("data-id"), 10), parseInt(f.qty.value, 10), parseFloat(f.cash.value));
        };
    });
    var rf = document.getElementById("restockForm");
    if (rf) rf.onsubmit = function (e) {
        e.preventDefault();
        restock(parseInt(rf.id.value, 10), parseInt(rf.amount.value, 10));
    };
    var af = document.getElementById("addForm");
    if (af) af.onsubmit = function (e) {
        e.preventDefault();
        addProduct(parseInt(af.id.value, 10), af.name.value.trim(), parseFloat(af.price.value), parseInt(af.stock.value, 10));
    };
}

function processSale(id, qty, cash) {
    if (!canDo(role, "sale")) return messageView("Access denied", "Only Cashier, Customer, and Supervisor can process sales.");
    var idx = findIndex(id);
    if (idx === -1) return messageView("Sale failed", "No item matched that ID.");
    var it = data.items[idx];
    if (it.stock === 0) return messageView("Sale failed", "Sorry, this item is out of stock.");
    if (isNaN(qty) || qty <= 0) return messageView("Sale failed", "Invalid quantity entered.");
    if (qty > it.stock) return messageView("Sale failed", "Error: Not enough stock available.");
    if (isNaN(cash) || cash < 0) return messageView("Sale failed", "Negative cash is not allowed.");

    var subtotal = it.price * qty;
    if (cash < subtotal) return messageView("Sale failed", "Cash is not enough! Transaction failed.");

    var change = cash - subtotal;
    it.stock -= qty;
    data.totalTransactions++;
    data.totalRevenue += subtotal;
    saveData();

    var receipt = "<div class=\"receipt\"><b>RECEIPT</b><br>" +
        "Item ID: " + it.id + "<br>" +
        "Item: " + safeText(it.name) + "<br>" +
        "Price: " + money(it.price) + " x " + qty + "<br>" +
        "Total: " + money(subtotal) + "<br>" +
        "Cash: " + money(cash) + "<br>" +
        "Change: " + money(change) + "<br>" +
        "Stock left: " + it.stock + "</div>";
    if (it.stock <= 5) {
        receipt += "<p style=\"color:var(--warn)\">LOW STOCK ALERT: only 5 or fewer left of " + safeText(it.name) + "!</p>";
    }
    receipt += "<br>";
    messageView("Transaction Successful!", "Thank you for your purchase.", receipt);
}

function restock(id, add) {
    if (!canDo(role, "restock")) return messageView("Access denied", "Only Inventory Admin and Supervisor can restock products.");
    var idx = findIndex(id);
    if (idx === -1) return messageView("Restock failed", "No item matched that ID.");
    if (isNaN(add) || add < 1 || add > 30) return messageView("Restock failed", "Invalid input, (1-30) items are only accepted.");
    data.items[idx].stock += add;
    saveData();
    messageView("Restocked", "Restock success! " + safeText(data.items[idx].name) + " new stock: " + data.items[idx].stock);
}

function addProduct(id, name, price, stock) {
    if (!canDo(role, "add")) return messageView("Access denied", "Only Inventory Admin and Supervisor can add new products.");
    if (data.items.length >= MAX) return messageView("Add failed", "Product list is already full (max " + MAX + " items).");
    if (name === "") return messageView("Add failed", "Name cannot be blank.");
    if (isNaN(id) || findIndex(id) !== -1) return messageView("Add failed", "That ID is already used by another item.");
    if (isNaN(price) || price <= 0 || isNaN(stock) || stock < 1 || stock > 30) {
        return messageView("Add failed", "Invalid input, (1-30) items are only accepted.");
    }
    data.items.push({ id: id, name: name, price: price, stock: stock });
    saveData();
    messageView("Product added", safeText(name) + " added! Total products now: " + data.items.length);
}

function setupDashboard() {
    view = document.getElementById("view");
    role = params().role || "";
    if (!validRole(role)) { location.href = "index.html"; return; }
    document.getElementById("dashLink").href = "dashboard.html?role=" + role;
    startTimer();
    renderDashboard();
}

if (document.getElementById("customerForm")) setupHome();
if (document.getElementById("view")) setupDashboard();
