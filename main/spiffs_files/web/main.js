
const tmpStripBox = document.getElementById("tmpStripBox");
const tmpZoneBox = document.getElementById("tmpZoneBox");
const tmpZoneNewBox = document.getElementById("tmpZoneNewBox");
const tmpCheckBox = document.getElementById("tmpCheckBox");
const containerBox = document.getElementById("containerBox");
const deleteDialog = new DeleteDialog();
const ws = new WebSocketHandler();
const com = new MessageHandler();
const stripList = [];
let stripMaxIndex = 0;
let pageHeader;

window.onload = onPageLoaded;

function onPageLoaded()
{
    pageHeader = document.getElementById("pageHeader");
    ws.newSocket();
}

function refreshStripConfig(isFirst, data) {
    let dataIndex = 0;
    let stripIndex = 0;
    let strip;
    let renderStrip;
    let dataView = new DataView(data.buffer);
    /* strip_n * (pixel_n + rgb_order) */

    while(dataIndex < (data.length)) {
        if(isFirst) renderStrip = addNewStrip(stripIndex);
        else renderStrip = stripList[stripIndex];

        strip = renderStrip.strip;

        // next byte is strip_n
        strip.pixelSize = dataView.getUint32(dataIndex);
        console.log(`pixel size: ${strip.pixelSize}`);
        dataIndex += 4;
        strip.rgbOrder = new TextDecoder().decode(data.slice(dataIndex, dataIndex + 3));
        console.log(`rgb order: ${strip.rgbOrder}`);
        dataIndex += 3;
        renderStrip.syncStripData();
        renderStrip.checkRemainPlace();
        stripIndex++;
    }

    if(isFirst) delDefText();
}

function addNewStrip(stripIndex) {
    stripMaxIndex++;
    const strip = new Strip(stripIndex);
    const tmpHTML = tmpStripBox.content.cloneNode(true).firstElementChild;
    const renderStrip = new RenderStrip(strip, tmpHTML);
    containerBox.appendChild(tmpHTML);
    stripList.push(renderStrip);
    return renderStrip;
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
