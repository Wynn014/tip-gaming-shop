#include "httplib.h"
#include <iostream>
#include <string>

using namespace std;

const int MAX = 10; // 10 slots, extra space is for the new products we add later

// these used to be inside main, moved out so they stay alive between requests
string names[MAX] = {"Mech Keyboard", "Gaming Mouse", "1TB SSD"};
int ids[MAX] = {101, 102, 103};
double inventory[MAX][2] = {
    {2500.00, 20},
    {1200.00, 20},
    {3500.00, 15}
};
int itemCount = 3;
int totalTransactions = 0;
double totalRevenue = 0.0;
int failedAttempts = 0; // wrong logins, 3 and the staff login locks

// Returning function, -1 if the ID is not on the list
int findIndex(int ids[], int size, int searchId) {
    for (int i = 0; i < size; i++) {
        if (ids[i] == searchId) {
            return i;
        }
    }
    return -1;
}

// Returning function
double computeSubtotal(double price, int qty) {
    return price * qty;
}

// Call by reference
void deductStock(double &stockQty, int qty) {
    stockQty -= qty;
}

// 1 if the role is one of the four
int validRole(string role) {
    if (role == "Supervisor" || role == "InventoryAdmin" || role == "Cashier" || role == "Customer") {
        return 1;
    }
    return 0;
}

// same role rules as the console version, 1 = allowed
int canDo(string role, string action) {
    if (action == "sale") {
        if (role == "Supervisor" || role == "Cashier" || role == "Customer") return 1;
    } else if (action == "restock" || action == "add") {
        if (role == "InventoryAdmin" || role == "Supervisor") return 1;
    } else if (action == "report") {
        if (role == "Supervisor" || role == "InventoryAdmin" || role == "Cashier") return 1;
    }
    return 0;
}

// to_string gives 2500.000000, this cuts the last 4 digits so it shows P2500.00
string money(double n) {
    string s = to_string(n);
    return "P" + s.substr(0, s.length() - 4);
}

// makes a typed name safe to show on a page (so <b> or <script> typed as a name is not run as HTML)
string safeText(string s) {
    string out = "";
    for (int i = 0; i < (int)s.length(); i++) {
        if (s[i] == '<') out += "&lt;";
        else if (s[i] == '>') out += "&gt;";
        else if (s[i] == '&') out += "&amp;";
        else if (s[i] == '"') out += "&quot;";
        else out += s[i];
    }
    return out;
}

// top part of every page the C++ makes (head and nav bar)
string pageTop(string title, string role) {
    string h = "<!DOCTYPE html><html><head><meta charset=\"UTF-8\">";
    h += "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">";
    h += "<title>" + title + " - TIP Gaming Shop</title>";
    h += "<link rel=\"stylesheet\" href=\"/style.css\"></head><body>";
    h += "<header class=\"top\"><div class=\"bar\"><a class=\"brand\" href=\"/\">TIP Gaming Shop</a><nav>";
    h += "<a href=\"/\">Home</a>";
    h += "<a href=\"/dashboard?role=" + role + "\">Dashboard</a>";
    h += "<a href=\"/about.html\">About Us</a>";
    h += "<a href=\"/faqs.html\">FAQs</a>";
    h += "<a href=\"/contacts.html\">Contacts</a>";
    h += "<a href=\"/terms.html\">Terms &amp; Conditions</a>";
    h += "</nav></div></header><main>";
    return h;
}

string pageBottom() {
    string h = "</main><footer>TIP Gaming Shop, a school project for Introduction to Computing.</footer>";

    // inactivity timer: if the user does nothing for 2 minutes, go to the /timeout page
    // change 120000 to another number (in milliseconds) to make it shorter or longer
    h += R"(<script>
        var timer;
        function resetTimer() {
            clearTimeout(timer);
            timer = setTimeout(function () { window.location = "/timeout"; }, 120000);
        }
        document.onmousemove = resetTimer;
        document.onkeydown = resetTimer;
        document.onclick = resetTimer;
        resetTimer();
    </script>)";

    h += "</body></html>";
    return h;
}

// a page with just a message and a link back, used for errors and success
string messagePage(string role, string title, string msg, string backLink, string backText) {
    string h = pageTop(title, role);
    h += "<h1>" + title + "</h1><section class=\"card\">";
    h += "<p>" + msg + "</p>";
    h += "<a class=\"linkbtn\" href=\"" + backLink + "\">" + backText + "</a>";
    h += "</section>" + pageBottom();
    return h;
}

// the whole dashboard, what shows up depends on the role
string dashboardPage(string role) {
    string h = pageTop("Dashboard", role);
    h += "<div class=\"who\"><h1 style=\"margin:0\">Dashboard</h1>";
    h += "<span class=\"badge good\">" + role + "</span>";
    h += "<a class=\"linkbtn\" href=\"/\">Log out</a></div>";

    // product cards
    h += "<h2>Shop</h2><div class=\"shelf\">";
    for (int i = 0; i < itemCount; i++) {
        h += "<div class=\"item tone" + to_string(i % 5) + "\">";
        h += "<div class=\"pic\"><span>" + names[i].substr(0, 1) + "</span></div>";
        h += "<div class=\"info\"><h3>" + names[i] + "</h3>";
        h += "<p class=\"muted\">ID " + to_string(ids[i]) + " | Stock " + to_string((int)inventory[i][1]) + "</p>";
        h += "<div class=\"priceRow\"><b>" + money(inventory[i][0]) + "</b>";

        if (inventory[i][1] == 0) {
            h += "<span class=\"badge out\">Out of stock</span>";
        } else if (inventory[i][1] <= 5) {
            h += "<span class=\"badge low\">Low stock</span>";
        } else {
            h += "<span class=\"badge good\">In stock</span>";
        }
        h += "</div>";

        // buy form, only for roles that can sell
        if (canDo(role, "sale") == 1) {
            if (inventory[i][1] == 0) {
                h += "<button disabled>Sold out</button>";
            } else {
                h += "<form class=\"buyform\" method=\"post\" action=\"/sale\">";
                h += "<input type=\"hidden\" name=\"role\" value=\"" + role + "\">";
                h += "<input type=\"hidden\" name=\"id\" value=\"" + to_string(ids[i]) + "\">";
                h += "<input type=\"number\" name=\"qty\" min=\"1\" value=\"1\" required>";
                h += "<input type=\"number\" name=\"cash\" min=\"0\" step=\"0.01\" placeholder=\"Cash\" required>";
                h += "<button type=\"submit\">Buy</button></form>";
            }
        }
        h += "</div></div>";
    }
    h += "</div>";

    // restock and add forms, only for Inventory Admin and Supervisor
    if (canDo(role, "restock") == 1) {
        h += "<div class=\"grid2\">";

        h += "<section class=\"card\"><h2>Restock a product</h2>";
        h += "<form method=\"post\" action=\"/restock\">";
        h += "<input type=\"hidden\" name=\"role\" value=\"" + role + "\">";
        h += "<label>Item<select name=\"id\">";
        for (int i = 0; i < itemCount; i++) {
            h += "<option value=\"" + to_string(ids[i]) + "\">" + to_string(ids[i]) + " - " + names[i] + "</option>";
        }
        h += "</select></label>";
        h += "<label>Amount to add<input type=\"number\" name=\"amount\" min=\"1\" max=\"30\" required></label>";
        h += "<button type=\"submit\">Restock</button></form></section>";

        h += "<section class=\"card\"><h2>Add a new product</h2>";
        h += "<form method=\"post\" action=\"/add\">";
        h += "<input type=\"hidden\" name=\"role\" value=\"" + role + "\">";
        h += "<label>Product ID<input type=\"number\" name=\"id\" required></label>";
        h += "<label>Product name<input type=\"text\" name=\"name\" required></label>";
        h += "<label>Price (P)<input type=\"number\" name=\"price\" min=\"0\" step=\"0.01\" required></label>";
        h += "<label>Starting stock<input type=\"number\" name=\"stock\" min=\"1\" max=\"30\" required></label>";
        h += "<button type=\"submit\">Add product</button></form></section>";

        h += "</div>";
    }

    // daily report
    if (canDo(role, "report") == 1) {
        h += "<section class=\"card\"><h2>Daily sales report</h2>";
        h += "<p>Total transactions: <b>" + to_string(totalTransactions) + "</b></p>";
        h += "<p>Total revenue: <b>" + money(totalRevenue) + "</b></p>";
        h += "<p>Out of stock items:</p>";

        int outCheck = 0; // 0 means no zero-stock item found yet, becomes 1 once there is one
        for (int i = 0; i < itemCount; i++) {
            if (inventory[i][1] == 0) {
                h += "<p>- " + names[i] + "</p>";
                outCheck = 1;
            }
        }
        if (outCheck == 0) {
            h += "<p class=\"muted\">None. All items have stock.</p>";
        }
        h += "</section>";
    }

    h += pageBottom();
    return h;
}

int main() {
    httplib::Server svr;

    // serves index.html, about.html, style.css and the other plain pages in www
    svr.set_mount_point("/", "./www");

    // login form sends us here, same idea as the console login
    svr.Post("/login", [](const httplib::Request &req, httplib::Response &res) {
        string username = req.get_param_value("username");
        string password = req.get_param_value("password");

        if (username == "") {
            res.set_content(messagePage("", "Login failed", "Please enter a username.", "/", "Try again"), "text/html");
            return;
        }

        // any username that is not a staff account is a customer, no password needed
        if (username != "Supervisor" && username != "InventoryAdmin" && username != "Cashier") {
            string welcome = "Welcome, " + safeText(username) + "! System automatically identified you as a Customer.";
            res.set_content(messagePage("Customer", "Welcome", welcome, "/dashboard?role=Customer", "Continue to shop"), "text/html");
            return;
        }

        // staff account, so check the password
        if (failedAttempts >= 3) {
            res.set_content(messagePage("", "Locked", "3 failed password attempts. System locked, please restart the program.", "/", "Back to home"), "text/html");
            return;
        }

        string role = "";

        if (username == "Supervisor" && password == "SupV123") {
            role = "Supervisor";
        } else if (username == "InventoryAdmin" && password == "Admin123") {
            role = "InventoryAdmin";
        } else if (username == "Cashier" && password == "Cashier123") {
            role = "Cashier";
        }

        if (role == "") {
            failedAttempts++;
            res.set_content(messagePage("", "Login failed", "Wrong password! Attempts left: " + to_string(3 - failedAttempts), "/", "Try again"), "text/html");
            return;
        }

        failedAttempts = 0;
        res.set_redirect("/dashboard?role=" + role);
    });

    // the page you land on after being inactive too long
    svr.Get("/timeout", [](const httplib::Request &req, httplib::Response &res) {
        res.set_content(messagePage("", "Session ended", "Session ended due to inactivity. Please log in again.", "/", "Back to home"), "text/html");
    });

    // the page that has everything (menu options 1 to 5 from the console)
    svr.Get("/dashboard", [](const httplib::Request &req, httplib::Response &res) {
        string role = req.get_param_value("role");
        if (validRole(role) == 0) {
            res.set_redirect("/");
            return;
        }
        res.set_content(dashboardPage(role), "text/html");
    });

    // processSale
    svr.Post("/sale", [](const httplib::Request &req, httplib::Response &res) {
        string role = req.get_param_value("role");
        if (validRole(role) == 0) {
            res.set_redirect("/");
            return;
        }
        string back = "/dashboard?role=" + role;

        if (canDo(role, "sale") == 0) {
            res.set_content(messagePage(role, "Access denied", "Only Cashier, Customer, and Supervisor can process sales.", back, "Back to dashboard"), "text/html");
            return;
        }

        int itemId = stoi(req.get_param_value("id"));
        int qty = stoi(req.get_param_value("qty"));
        double cash = stod(req.get_param_value("cash"));

        int index = findIndex(ids, itemCount, itemId);

        if (index == -1) {
            res.set_content(messagePage(role, "Sale failed", "No item matched that ID.", back, "Back to dashboard"), "text/html");
            return;
        }
        if (inventory[index][1] == 0) {
            res.set_content(messagePage(role, "Sale failed", "Sorry, this item is out of stock.", back, "Back to dashboard"), "text/html");
            return;
        }
        if (qty <= 0) {
            res.set_content(messagePage(role, "Sale failed", "Invalid quantity entered.", back, "Back to dashboard"), "text/html");
            return;
        }
        if (qty > inventory[index][1]) {
            res.set_content(messagePage(role, "Sale failed", "Error: Not enough stock available.", back, "Back to dashboard"), "text/html");
            return;
        }

        double subtotal = computeSubtotal(inventory[index][0], qty);

        if (cash < 0) {
            res.set_content(messagePage(role, "Sale failed", "Negative cash is not allowed.", back, "Back to dashboard"), "text/html");
            return;
        }
        if (cash < subtotal) {
            res.set_content(messagePage(role, "Sale failed", "Cash is not enough! Transaction failed.", back, "Back to dashboard"), "text/html");
            return;
        }

        double change = cash - subtotal;

        // Call by reference to update stock
        deductStock(inventory[index][1], qty);

        totalTransactions++;
        totalRevenue += subtotal;

        string r = pageTop("Receipt", role);
        r += "<h1>Transaction Successful!</h1><section class=\"card\"><div class=\"receipt\"><b>RECEIPT</b><br>";
        r += "Item ID: " + to_string(ids[index]) + "<br>";
        r += "Item: " + names[index] + "<br>";
        r += "Price: " + money(inventory[index][0]) + " x " + to_string(qty) + "<br>";
        r += "Total: " + money(subtotal) + "<br>";
        r += "Cash: " + money(cash) + "<br>";
        r += "Change: " + money(change) + "<br>";
        r += "Stock left: " + to_string((int)inventory[index][1]) + "</div>";

        // alert goes here too so the cashier sees it right after the receipt
        if (inventory[index][1] <= 5) {
            r += "<p style=\"color:var(--warn)\">LOW STOCK ALERT: only 5 or fewer left of " + names[index] + "!</p>";
        }
        r += "<br><a class=\"linkbtn\" href=\"" + back + "\">Back to dashboard</a></section>";
        r += pageBottom();
        res.set_content(r, "text/html");
    });

    // restockProduct
    svr.Post("/restock", [](const httplib::Request &req, httplib::Response &res) {
        string role = req.get_param_value("role");
        if (validRole(role) == 0) {
            res.set_redirect("/");
            return;
        }
        string back = "/dashboard?role=" + role;

        if (canDo(role, "restock") == 0) {
            res.set_content(messagePage(role, "Access denied", "Only Inventory Admin and Supervisor can restock products.", back, "Back to dashboard"), "text/html");
            return;
        }

        int itemId = stoi(req.get_param_value("id"));
        int add = stoi(req.get_param_value("amount"));

        int index = findIndex(ids, itemCount, itemId);

        if (index == -1) {
            res.set_content(messagePage(role, "Restock failed", "No item matched that ID.", back, "Back to dashboard"), "text/html");
            return;
        }
        if (add < 1 || add > 30) {
            res.set_content(messagePage(role, "Restock failed", "Invalid input, (1-30) items are only accepted.", back, "Back to dashboard"), "text/html");
            return;
        }

        inventory[index][1] += add;
        res.set_content(messagePage(role, "Restocked", "Restock success! " + names[index] + " new stock: " + to_string((int)inventory[index][1]), back, "Back to dashboard"), "text/html");
    });

    // addProduct
    svr.Post("/add", [](const httplib::Request &req, httplib::Response &res) {
        string role = req.get_param_value("role");
        if (validRole(role) == 0) {
            res.set_redirect("/");
            return;
        }
        string back = "/dashboard?role=" + role;

        if (canDo(role, "add") == 0) {
            res.set_content(messagePage(role, "Access denied", "Only Inventory Admin and Supervisor can add new products.", back, "Back to dashboard"), "text/html");
            return;
        }
        if (itemCount >= MAX) {
            res.set_content(messagePage(role, "Add failed", "Product list is already full (max " + to_string(MAX) + " items).", back, "Back to dashboard"), "text/html");
            return;
        }

        int newId = stoi(req.get_param_value("id"));
        string newName = req.get_param_value("name");
        double newPrice = stod(req.get_param_value("price"));
        int newStock = stoi(req.get_param_value("stock"));

        if (newName == "") {
            res.set_content(messagePage(role, "Add failed", "Name cannot be blank.", back, "Back to dashboard"), "text/html");
            return;
        }
        if (findIndex(ids, itemCount, newId) != -1) {
            res.set_content(messagePage(role, "Add failed", "That ID is already used by another item.", back, "Back to dashboard"), "text/html");
            return;
        }
        if (newPrice <= 0 || newStock < 1 || newStock > 30) {
            res.set_content(messagePage(role, "Add failed", "Invalid input, (1-30) items are only accepted.", back, "Back to dashboard"), "text/html");
            return;
        }

        ids[itemCount] = newId;
        names[itemCount] = newName;
        inventory[itemCount][0] = newPrice;
        inventory[itemCount][1] = newStock;
        itemCount++;

        res.set_content(messagePage(role, "Product added", newName + " added! Total products now: " + to_string(itemCount), back, "Back to dashboard"), "text/html");
    });

    cout << "Server running at http://localhost:8080" << endl;
    svr.listen("0.0.0.0", 8080);
    return 0;
}