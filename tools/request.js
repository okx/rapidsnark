const fs = require("fs");
const fetch = require('node-fetch');

// If not enough arguments, print usage
if (process.argv.length < 3) {
    console.log("Usage: node request.js <input file> <circuit> [Optional: port]");
}
const input = fs.readFileSync(process.argv[2], "utf8");
const circuit = process.argv[3];
// If get the port from the command line, use it, otherwise use the default port
const port = process.argv[4] || 9080;

async function callInput() {
    const rawResponse = await fetch(`http://localhost:${port}/input/${circuit}`, {
      method: 'POST',
      headers: {
        'Accept': 'application/json',
        'Content-Type': 'application/json'
      },
      body: input
    });
    if (rawResponse.ok) {
        return true;
    } else {
        throw new Error(rawResponse.status);
    }
};


async function getStatus() {
    const rawResponse = await fetch(`http://localhost:${port}/status`, {
        method: 'GET',
        headers: {
            'Accept': 'application/json'
        }
    });
    if (!rawResponse.ok) {
        throw new Error(rawResponse.status);
    }
    return rawResponse.json();
}

async function run() {
    await callInput();
    let st;
    st = await getStatus();
    while (st.status == "busy") {
        st = await getStatus();
    }
    console.log(JSON.stringify(st, null,1));
}

run().then(() => {
    process.exit(0);
}, (err) => {
    console.log("ERROR");
    console.log(err);
    process.exit(1);
});