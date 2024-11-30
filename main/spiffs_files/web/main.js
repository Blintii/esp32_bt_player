
const tmpModbusBox = document.getElementById("tmpModbusBox");
const tmpControlItem = document.getElementById("tmpControlItem");
const tmpCheckBox = document.getElementById("tmpCheckBox");
const tmpLED = document.getElementById("tmpLED");
const deleteDialog = new DeleteDialog();
const ws = new WebSocketHandler();
const com = new MessageHandler();
const deviceList = [];
let deviceMaxN = 0;
let pageHeader;

window.onload = onPageLoaded;

function onPageLoaded()
{
    pageHeader = document.getElementById("pageHeader");
    ws.newSocket();
}

function refreshModbusDevices(isFirst, data) {
    let dataIndex = 0;
    let deviceIndex = 0;
    let modbusDevice;
    let renderModbusDevice;

    while(dataIndex < (data.length)) {
        if(isFirst) renderModbusDevice = addNewModbusDevice(deviceIndex);
        else renderModbusDevice = deviceList[deviceIndex];

        modbusDevice = renderModbusDevice.device;

        // if next byte TRUE: device exist on ESP
        if(data[dataIndex++])
        {
            // if device not exist yet in UI, need to setup
            if(!modbusDevice.exist) {
                modbusDevice.exist = true
                renderModbusDevice.setupControls();
            }

            modbusDevice.address = data[dataIndex++].toString(16).toUpperCase();

            if(modbusDevice.address.length == 1) modbusDevice.address = "0" + modbusDevice.address;

            setBitValues(modbusDevice.coils, data[dataIndex++]);
            setBitValues(modbusDevice.inputs, data[dataIndex++]);
            renderModbusDevice.syncDeviceData();
        }
        else
        {
            // if device still exist in UI, need to delete
            if(modbusDevice.exist) {
                modbusDevice.exist = false;
                renderModbusDevice.deleteControls();
            }
        }

        deviceIndex++;
    }

    if(isFirst) delDefText();
}

function addNewModbusDevice(deviceIndex) {
    deviceMaxN++;
    const modbusDevice = new ModbusDevice(deviceIndex, 8);
    const tmpHTML = tmpModbusBox.content.cloneNode(true).firstElementChild;
    const renderModbusDevice = new RenderModbusDevice(modbusDevice, tmpHTML);
    renderModbusDevice.setDeviceName(deviceIndex);
    document.body.appendChild(tmpHTML);
    deviceList.push(renderModbusDevice);
    return renderModbusDevice;
}

function setBitValues(boolArray, byteData) {
    for(let i = 0; i < 8; i++) {
        if(byteData & 1) boolArray[i] = true;
        else boolArray[i] = false;

        byteData = byteData >> 1;
    }
}

function delDefText() {
    const defText = document.getElementById("default_text");

    if(defText) defText.remove();
}

function showHeader(text, color, timeMS) {
    let textDiv = document.getElementById("headerText");
    textDiv.textContent = text;
    pageHeader.style.display = "block";
    pageHeader.style.backgroundColor = color;

    if(pageHeader.curTimeoutID) {
        clearTimeout(pageHeader.curTimeoutID);
        pageHeader.curTimeoutID = null;
    }

    if(timeMS) pageHeader.curTimeoutID = setTimeout(hideHeader, timeMS);
}

function hideHeader() {
    pageHeader.style.display = "none";
}
