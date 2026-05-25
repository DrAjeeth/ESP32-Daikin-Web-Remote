// ======================================================
// UPDATE DATA
// ======================================================

async function updateData(){

let response = await fetch("/data");

let data = await response.json();

window.autoModeEnabled =
data.autoMode;

document.getElementById("clock").innerHTML =
data.time;

// ======================================================
// SENSOR DATA
// ======================================================

document.getElementById("temp").innerHTML =
data.temperature + " °C";

document.getElementById("pressure").innerHTML =
data.pressure + " hPa";

// ======================================================
// SET TEMP
// ======================================================

document.getElementById("settemp").innerHTML =
data.setTemp;

// ======================================================
// AC STATE
// ======================================================

document.getElementById("acstate").innerHTML =
data.acPower ? "ON" : "OFF";

// ======================================================
// MODE DISPLAY
// ======================================================

if(data.acMode == 3){

document.getElementById("mode").innerHTML =
"COOL";
}

else if(data.acMode == 2){

document.getElementById("mode").innerHTML =
"DRY";
}

else {

document.getElementById("mode").innerHTML =
"FAN";
}

// ======================================================
// FAN DISPLAY
// ======================================================

if(data.quietMode){

document.getElementById("fan").innerHTML =
"🌲";
}

else if(data.fanSpeed == 10){

document.getElementById("fan").innerHTML =
"AUTO";
}

else {

document.getElementById("fan").innerHTML =
data.fanSpeed;
}

// ======================================================
// SLEEP STATUS
// ======================================================

document.getElementById("sleepstatus").innerHTML =
data.sleepMode ? "RUNNING" : "OFF";

// ======================================================
// CYCLE STATUS
// ======================================================

document.getElementById("cyclestatus").innerHTML =
data.cycleMode ? "RUNNING" : "OFF";

// ======================================================
// CYCLE COUNT
// ======================================================

document.getElementById("cyclecount").innerHTML =
data.completedCycles + " / " +
data.totalCycles;

// ======================================================
// AUTO MODE STATUS
// ======================================================

document.getElementById("automodestatus").innerHTML =
data.autoMode ? "ON" : "OFF";

setInputValue("autoon", data.autoOnTemp);

setInputValue("autooff", data.autoOffTemp);

// ======================================================
// ADVANCED FEATURE BUTTON STATES
// ======================================================

setActive("powerfulbtn", data.powerfulMode);

setActive("econobtn", data.econoMode);

setActive("quietbtn", data.quietMode);

setActive("swinghbtn", data.swingHorizontal);

setActive("coandabtn", data.coandaMode);

setActive("swingvbtn", data.swingVertical);

setActive("automodebtn", data.autoMode);
}

function setActive(id, enabled){

let button =
document.getElementById(id);

if(!button){

return;
}

button.classList.toggle(
"active",
enabled === true || enabled === "true" || enabled === 1);
}

function setInputValue(id, value){

let input =
document.getElementById(id);

if(!input || document.activeElement === input){

return;
}

input.value =
value;
}

async function sendCommand(url){

await fetch(url);

updateData();
}

// ======================================================
// BASIC POWER
// ======================================================

function acOn(){

fetch("/on");
}

function acOff(){

fetch("/off");
}

// ======================================================
// TEMPERATURE
// ======================================================

function tempUp(){

fetch("/tempup");
}

function tempDown(){

fetch("/tempdown");
}

// ======================================================
// FAN SPEED
// ======================================================

function setFan(speed){

fetch("/fan?speed=" + speed);
}

// ======================================================
// MODE
// ======================================================

function setMode(mode){

fetch("/mode?type=" + mode);
}

// ======================================================
// AUTO OFF TIMER
// ======================================================

function autoOff(minutes){

fetch("/autoff?minutes=" + minutes);
}

// ======================================================
// AUTO MODE
// ======================================================

function startAutoMode(){

let on =
document.getElementById("autoon").value;

let off =
document.getElementById("autooff").value;

sendCommand("/automode?enable=1"
+"&on="+on
+"&off="+off);
}

function stopAutoMode(){

sendCommand("/automode?enable=0");
}

function toggleAutoMode(){

if(window.autoModeEnabled){

stopAutoMode();

} else {

startAutoMode();
}
}

// ======================================================
// ADVANCED SLEEP MODE
// ======================================================

function startSleep(){

let h =
document.getElementById("sleephours").value;

let max =
document.getElementById("sleepmax").value;

fetch("/sleep?enable=1"
+"&hours="+h
+"&max="+max);
}

function stopSleep(){

fetch("/sleep?enable=0");
}

// ======================================================
// CYCLICAL TIMER
// ======================================================

function startCycle(){

let on =
document.getElementById("cycleon").value;

let off =
document.getElementById("cycleoff").value;

let count =
document.getElementById("cyclenum").value;

fetch("/cycle?enable=1"
+"&on="+on
+"&off="+off
+"&count="+count);
}

function stopCycle(){

fetch("/cycle?enable=0");
}

// ======================================================
// ADVANCED FEATURES
// ======================================================

function togglePowerful(){

sendCommand("/powerful");
}

function toggleEcono(){

sendCommand("/econo");
}

function toggleQuiet(){

sendCommand("/quiet");
}

function toggleSwingV(){

sendCommand("/swingv");
}

function toggleSwingH(){

sendCommand("/swingh");
}

function toggleCoanda(){

sendCommand("/coanda");
}

// ======================================================
// AUTO REFRESH
// ======================================================

setInterval(updateData,2000);

updateData();
