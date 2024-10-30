/**
 * ----------------------------------------------------------------------------
 * Bell Alarm Clock
 * ----------------------------------------------------------------------------
 * © 2024 Hans Weda
 * ----------------------------------------------------------------------------
 */

var gateway = `ws://${window.location.hostname}/ws`;
var websocket;

// ----------------------------------------------------------------------------
// Initialization
// ----------------------------------------------------------------------------

window.addEventListener('load', onLoad);

window.addEventListener('load', refreshTime, setInterval(refreshTime, 2000));

function onLoad(event) {
  initWebSocket();
  initButtons();
}


// ----------------------------------------------------------------------------
// WebSocket handling
// ----------------------------------------------------------------------------

function initWebSocket() {
  console.log('Trying to open a WebSocket connection...');
  websocket = new WebSocket(gateway);
  websocket.onopen  = onOpen;
  websocket.onclose = onClose;
  websocket.onmessage = onMessage;
}

function onOpen(event) {
  console.log('Connection opened');
}

function onClose(event) {
  console.log('Connection closed');
  setTimeout(initWebSocket, 2000);
}

function onMessage(event) {
  let data = JSON.parse(event.data);
  document.getElementById('motor').checked = data.motorButtonOn;
  document.getElementById('alarm').checked = data.alarmButtonOn;
  document.getElementById('alarm_time').innerHTML = data.alarmTime;
  document.getElementById('time').value = data.alarmTime;
  switch(data.motorStatus) {
    case 0: 
      document.getElementById("bell").classList.remove("running_bell", "stopping_bell"); 
      document.getElementById("bell").classList.add("stopped_bell"); 
      break;
    case 1: 
      document.getElementById("bell").classList.remove("stopping_bell", "stopped_bell"); 
      document.getElementById("bell").classList.add("running_bell"); 
      break;
    case 2: 
      document.getElementById("bell").classList.remove("running_bell", "stopped_bell"); 
      document.getElementById("bell").classList.add("stopping_bell"); 
      break;
  };
}


// ----------------------------------------------------------------------------
// Button handling
// ----------------------------------------------------------------------------

function initButtons() {
  // Motor button
  document.getElementById('motor').addEventListener('click', submitData);
  // Alarm Button
  document.getElementById('alarm').addEventListener('click', submitData);
  // Open modal
  document.getElementById('set_time').addEventListener('click', openModal);
  // Submit time
  document.getElementById('submit_time').addEventListener('click', function(event) {submitData(event); closeModal(event)})
  // Close modal by close button
  document.getElementsByClassName('close')[0].addEventListener('click', closeModal);
  // Close modal by clicking outside modal
  var modal = document.getElementById('set_time_modal');
  window.onclick = function(event) { if (event.target == modal) { closeModal(event) }};
}

function submitData(event) {
  websocket.send(
    JSON.stringify({
      'motorButtonOn': document.getElementById('motor').checked,
      'alarmButtonOn': document.getElementById('alarm').checked,
      'alarmTime': document.getElementById('time').value
    })
  );
}

function openModal(event) {
  document.getElementById('set_time_modal').style.display = 'block';
}

function closeModal(event) {
  document.getElementById('set_time_modal').style.display = 'none';
}


// ----------------------------------------------------------------------------
// Refreshing time
// ----------------------------------------------------------------------------

function refreshTime() {
  // calculate remaining time
  const formTime = document.getElementById('alarm_time').innerHTML
  const localTime = new Date(Date.now());
  if (formTime != '') {
    const alarmTime = new Date(localTime);
    alarmTime.setSeconds(0);
    alarmTime.setMinutes(formTime.split(':')[1]);
    alarmTime.setHours(formTime.split(':')[0]);
    document.getElementById('remaining_time').innerHTML = (alarmTime - localTime)/1000;
  } else {
    document.getElementById('remaining_time').innerHTML = '-';
  };
  document.getElementById('current_time').innerHTML = localTime.toLocaleString();
}