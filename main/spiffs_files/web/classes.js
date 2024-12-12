
const SHADER_SINGLE = 0;
const SHADER_REPEAT = 1;
const SHADER_FADE = 2;
const SHADER_FFT = 3;

const regexNumbers = /[^0-9]+/;
const regexRGB = /[^RGBrgb]+/;

class Strip {
    constructor(id) {
        this.id = id;
        this.pixelSize = 0;
        this.pixelUsedPos = 0;
        this.rgbOrder = "RGB";
        this.zones = [];
    }

    createZone() {
        let zone = new Zone(this.zones.length, this);
        zone.pixelSize = 1;
        this.pixelUsedPos += 1;
        this.zones.push(zone);
        return zone;
    }

    deleteZone() {
        let zone = this.zones.pop();

        if(zone) this.pixelUsedPos -= zone.pixelSize;
    }
}

class Zone {
    constructor(id, strip) {
        this.id = id;
        this.strip = strip;
        this.pixelSize = 0;
        this.shaderType = SHADER_SINGLE;
    }

    setPixelSize(size) {
        this.strip.pixelUsedPos -= this.pixelSize;
        this.pixelSize = size;
        this.strip.pixelUsedPos += size;
    }
}

class RenderStrip {
    constructor(strip, htmlBox) {
        this.strip = strip;
        this.htmlBox = htmlBox;
        let stripSizeBox = htmlBox.getElementsByClassName("stripSizeBox")[0];
        this.stripSizeTyper = new WriteTyper(stripSizeBox, regexNumbers, 3, newVal => this.onSizeTyperDone(newVal));
        let rgbOrderBox = htmlBox.getElementsByClassName("RGBOrderBox")[0];
        this.rgbOrderTyper = new WriteTyper(rgbOrderBox, regexRGB, 3, newVal => this.onRgbOrderTyperDone(newVal));
        this.newZoneHidden = true;
        this.uiControlsBox = htmlBox.getElementsByClassName("uiControls")[0];
        this.uiZoneNewBox = tmpZoneNewBox.content.cloneNode(true).firstElementChild;
        this.uiZoneNewBox.onclick = () => this.createRenderZone();
        this.renderZones = [];
        this.setStripTitle(this.strip.id + ". LED szalag");
    }

    setStripTitle(text) {
        const stripTitle = this.htmlBox.getElementsByClassName("stripTitle")[0];
        stripTitle.textContent = text;
    }

    syncStripData() {
        this.stripSizeTyper.textBox.textContent = this.strip.pixelSize;
        this.rgbOrderTyper.textBox.textContent = this.strip.rgbOrder;
    }

    checkRemainPlace() {
        if(this.strip.pixelUsedPos < this.strip.pixelSize) {
            if(this.newZoneHidden) {
                this.newZoneHidden = false;
                this.uiControlsBox.appendChild(this.uiZoneNewBox);
            }
        }
        else if(!this.newZoneHidden) {
            this.newZoneHidden = true;
            this.uiControlsBox.removeChild(this.uiZoneNewBox);
        }
    }

    onSizeTyperDone(newVal) {
        let size = parseInt(newVal, 10);

        if(size < this.strip.pixelUsedPos) {
            size = this.strip.pixelUsedPos;
        }

        if(this.strip.pixelSize != size) {
            this.strip.pixelSize = size;
            com.serverBound_stripSet(this.strip.id, this.strip.pixelSize, this.strip.rgbOrder);
            this.checkRemainPlace();
        }

        return size;
    }

    onRgbOrderTyperDone(newVal) {
        if(!(newVal.includes('R') && newVal.includes('G') && newVal.includes('B'))) {
            newVal = "RGB";
        }

        if(newVal != this.strip.rgbOrder) {
            this.strip.rgbOrder = newVal
            com.serverBound_stripSet(this.strip.id, this.strip.pixelSize, this.strip.rgbOrder);
        }

        return newVal;
    }

    createRenderZone() {
        this.newZoneHidden = true;
        this.newZonePixelSet = true;
        this.uiControlsBox.removeChild(this.uiZoneNewBox);
        let zoneHtmlBox = tmpZoneBox.content.cloneNode(true).firstElementChild;
        this.uiControlsBox.appendChild(zoneHtmlBox);
        let zone = new RenderZone(this.strip.createZone(), zoneHtmlBox, this);
        this.renderZones.push(zone);
        zone.syncZoneData();
        this.checkRemainPlace();
    }

    deleteRenderZone() {
        this.renderZones.pop();
        this.strip.deleteZone();
        this.checkRemainPlace();
    }
}

class RenderZone {
    constructor(zone, htmlBox, renderStrip) {
        this.zone = zone;
        this.htmlBox = htmlBox;
        this.renderStrip = renderStrip;
        let zoneSizeBox = htmlBox.getElementsByClassName("zoneSizeBox")[0];
        this.zoneSizeTyper = new WriteTyper(zoneSizeBox, regexNumbers, 3, newVal => this.onSizeTyperDone(newVal));
        this.deleteBox = htmlBox.getElementsByClassName("uiZoneDelete")[0];
        this.deleteBox.onclick = () => this.onDelete();
        this.setZoneTitle(`${this.zone.id}. zóna`);
    }

    setZoneTitle(text) {
        const zoneTitle = this.htmlBox.getElementsByClassName("zoneTitle")[0];
        zoneTitle.textContent = text;
    }

    syncZoneData() {
        this.zoneSizeTyper.textBox.textContent = this.zone.pixelSize;
    }

    onSizeTyperDone(newVal) {
        if(0 < newVal) {
            let newSize = parseInt(newVal, 10);
            let maxAllowed = this.zone.strip.pixelUsedPos - this.zone.pixelSize;

            if(maxAllowed < newSize) newSize = maxAllowed;

            if(newSize != this.zone.pixelSize) {
                this.zone.setPixelSize();
                com.serverBound_zoneSet(this.zone.strip.id, this.zone.id, this.zone.pixelSize);
                this.renderStrip.checkRemainPlace();
            }

            return newSize;
        }
        else return 1;
    }

    onDelete() {
        let text = `${this.zone.id} zóna törlése?\n(${this.zone.pixelSize} pixel)`;
        deleteDialog.show(() => {
            this.deleteBox.onclick = null;
            showHeader(`Zóna ${this.zone.id} törölve`, "rgb(190,30,0)", 3000);
            this.renderStrip.deleteRenderZone();
            this.htmlBox.remove();
        }, text);
    }
}

class WriteTyper {
    #startZerosPattern = /\b(0(?!\b))+/g;

    constructor(parentBox, checkPattern, maxCharLen, onDone) {
        this.parentBox = parentBox;
        this.checkPattern = checkPattern;
        this.maxCharLen = maxCharLen;
        this.onDone = onDone;
        this.textBox = parentBox.getElementsByClassName("uiWriteTyperText")[0];
        this.textBox.onblur = () => this.endType();
        this.textBox.onpaste = (event) => event.preventDefault();
        this.textBox.oninput = (event) => {
            this.check();
            // detect ENTER pressed
            if(event.inputType == "insertParagraph" && event.data == null) {
                document.activeElement.blur();
            }
            else this.setCaretPosition();
        };
        this.buttonSet = parentBox.getElementsByClassName("uiWriteTyperSet")[0];
        this.parentBox.onmousedown = (event) => {
            if(this.typing) event.preventDefault();
        };
        this.parentBox.onclick = (event) => {
            if(this.typing) {
                // if edit button clicked -> force trigger focus out (blur) event
                if(this.buttonSet.contains(event.target)) {
                    document.activeElement.blur();
                }
            }
            else this.startType();
        };
    }

    startType() {
        this.typing = true;
        this.originalValue = this.textBox.textContent;
        this.textBox.focus();
        this.check();
        this.setCaretPosition();
    }

    endType() {
        this.typing = false;
        let newVal = this.textBox.textContent;

        if(newVal.length == 0) newVal = "0";
        else newVal = newVal.replace(this.#startZerosPattern, '');

        this.textBox.textContent = newVal;

        if(newVal != this.originalValue) {
            this.textBox.textContent = this.onDone(newVal);
        }
    }

    setCaretPosition() {
        let sel = document.getSelection();
        sel.modify("move", "forward", "lineboundary");
    }

    check() {
        let s = this.textBox.textContent;
        s = s.replace(this.checkPattern, "");

        if(s.length > this.maxCharLen) s = s.substring(1, this.maxCharLen + 1);

        this.textBox.textContent = s.toUpperCase();
    }
}

class DeleteDialog {
    constructor()
    {
        this.htmlBox = document.getElementById("deleteDialog");
        this.deleteButton = document.getElementById("deleteButtonConfirm");
        this.htmlBox.onclick = (event) => this.click(event);
    }

    show(deleteTask, text) {
        this.deleteTask = deleteTask;
        this.htmlBox.showModal();
        this.htmlBox.focus();
        this.htmlBox.classList.add("openedDialog");
        let textBox = document.getElementById("deleteText");
        textBox.innerText = text;
    }

    click(event) {
        this.htmlBox.classList.remove("openedDialog");

        if(event.target == this.deleteButton) {
            if(this.deleteTask) {
                try {
                    this.deleteTask();
                }
                catch(e) {console.error(e)}
            }

            setTimeout(() => {
                this.htmlBox.close();
            }, 300);
        }
        else setTimeout(() => this.htmlBox.close(), 300);
    }
}
