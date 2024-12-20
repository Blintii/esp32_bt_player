
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
        this.stripSizeTyper = new WriteTyper(stripSizeBox, regexNumbers, 3);
        this.stripSizeTyper.onChanged = newVal => this.onSizeTyperDone(newVal);
        this.stripSizeTyper.onFinalCheck = newVal => this.onSizeTyperCheck(newVal);
        let rgbOrderBox = htmlBox.getElementsByClassName("RGBOrderBox")[0];
        this.rgbOrderTyper = new WriteTyper(rgbOrderBox, regexRGB, 3);
        this.rgbOrderTyper.onChanged = newVal => this.onRgbOrderTyperDone(newVal);
        this.rgbOrderTyper.onFinalCheck = newVal => this.onRgbOrderTyperCheck(newVal);
        this.newZoneHidden = true;
        this.zoneListBox = htmlBox.getElementsByClassName("stripZoneList")[0];
        this.renderZones = [];
        this.setStripTitle(`LED szalag ${this.strip.id}`);
    }

    setStripTitle(text) {
        const stripTitle = this.htmlBox.getElementsByClassName("stripTitle")[0];
        stripTitle.textContent = text;
    }

    syncStripData() {
        this.stripSizeTyper.textBox.textContent = this.strip.pixelSize;
        this.rgbOrderTyper.textBox.textContent = this.strip.rgbOrder;
    }

    checkCanShowCreate() {
        if(this.strip.pixelUsedPos < this.strip.pixelSize) {
            if(this.newZoneHidden) {
                this.newZoneHidden = false;
                let builderBox = tmpStripZoneListItem.content.cloneNode(true).firstElementChild;
                this.zoneNewBuilder = new RenderZoneBuilder(builderBox, this, this.zoneListBox);
            }
        }
        else if(!this.newZoneHidden) {
            this.newZoneHidden = true;
            this.zoneNewBuilder.cancel();
            this.zoneNewBuilder = null;
        }

        if(0 < this.renderZones.length) {
            let i = 0;
            let buttonBox;

            while(i < this.renderZones.length - 1) {
                buttonBox = this.renderZones[i].stripZoneListItemBox.getElementsByClassName("zoneDeleteButton")[0];
                buttonBox.classList.add("buttonDisabled");
                i++;
            }

            buttonBox = this.renderZones[i].stripZoneListItemBox.getElementsByClassName("zoneDeleteButton")[0];
            buttonBox.classList.remove("buttonDisabled");
        }
    }

    onSizeTyperCheck(newVal) {
        if(newVal.length == 0) newVal = "0";

        let newSize = parseInt(newVal, 10);

        if(newSize < this.strip.pixelUsedPos) {
            newSize = this.strip.pixelUsedPos;
        }

        return newSize;
    }

    onSizeTyperDone(newVal) {
        this.strip.pixelSize = newVal;
        com.serverBound_stripSet(this.strip.id, this.strip.pixelSize, this.strip.rgbOrder);
        this.checkCanShowCreate();
    }

    onRgbOrderTyperCheck(newVal) {
        if(!(newVal.includes('R') && newVal.includes('G') && newVal.includes('B'))) {
            newVal = "RGB";
        }

        return newVal;
    }

    onRgbOrderTyperDone(newVal) {
        this.strip.rgbOrder = newVal
        com.serverBound_stripSet(this.strip.id, this.strip.pixelSize, this.strip.rgbOrder);
    }

    createRenderZone() {
        this.newZoneHidden = true;
        let zoneHtmlBox = tmpZoneBox.content.cloneNode(true).firstElementChild;
        contentContainer.appendChild(zoneHtmlBox);
        let zone = new RenderZone(this.strip.createZone(), zoneHtmlBox, this.zoneNewBuilder.htmlBox, this);
        this.renderZones.push(zone);
        this.zoneNewBuilder = null;
        this.checkCanShowCreate();
        return zone;
    }

    deleteRenderZone() {
        this.renderZones.pop();
        this.strip.deleteZone();
        this.checkCanShowCreate();
    }
}

class RenderZoneBuilder {
    constructor(htmlBox, renderStrip, zoneListBox) {
        this.htmlBox = htmlBox;
        this.renderStrip = renderStrip;
        this.zoneListBox = zoneListBox;
        this.htmlBox.getElementsByClassName("zoneTitle")[0].textContent = "új zóna";
        this.zoneListBox.appendChild(this.htmlBox);
        let zoneSizeBox = this.htmlBox.getElementsByClassName("zoneSizeBox")[0];
        this.zoneSizeTyper = new WriteTyper(zoneSizeBox, regexNumbers, 3);
        this.zoneSizeTyper.onChanged = newVal => this.onSizeTyperDone(newVal);
        this.zoneSizeTyper.onFinalCheck = newVal => this.onSizeTyperCheck(newVal);
    }

    onSizeTyperCheck(newVal) {
        if(newVal.length == 0) newVal = "0";

        let newSize = parseInt(newVal, 10);

        if(0 < newSize) {
            let strip = this.renderStrip.strip;
            let maxAllowed = strip.pixelSize - strip.pixelUsedPos;

            if(maxAllowed < newSize) newSize = maxAllowed;
        }
        else newSize = 0;

        return newSize;
    }

    onSizeTyperDone(newVal) {
        let renderZone = this.renderStrip.createRenderZone();
        renderZone.onSizeTyperDone(newVal);
        renderZone.syncZoneData();
    }

    cancel() {
        this.htmlBox.remove();
    }
}

class RenderZone {
    constructor(zone, htmlBox, stripZoneListItemBox, renderStrip) {
        this.zone = zone;
        this.htmlBox = htmlBox;
        this.stripZoneListItemBox = stripZoneListItemBox;
        this.renderStrip = renderStrip;
        let zoneSizeBox = stripZoneListItemBox.getElementsByClassName("zoneSizeBox")[0];
        this.zoneSizeTyper = new WriteTyper(zoneSizeBox, regexNumbers, 3);
        this.zoneSizeTyper.onChanged = newVal => this.onSizeTyperDone(newVal);
        this.zoneSizeTyper.onFinalCheck = newVal => this.onSizeTyperCheck(newVal);
        this.deleteBox = stripZoneListItemBox.getElementsByClassName("zoneDeleteButton")[0];
        this.deleteBox.onclick = () => this.onDelete();
        this.setZoneTitle(`zóna ${this.zone.id}`);
        this.htmlBox.getElementsByClassName("zoneTitleParentRef")[0].textContent = `LED szalag ${this.renderStrip.strip.id}`;
        this.zoneType = htmlBox.getElementsByClassName("zoneType")[0];
        // TODO zoneType handling
        this.zoneType.onchange = () => console.log(this.zoneType.value);
        let callback = throttle(() => {
            this.shaderConfig = {};
            this.shaderConfig.colorHSL = {};
            this.shaderConfig.colorHSL.hue = this.colorPicker.hue/Math.PI*180;
            this.shaderConfig.colorHSL.sat = this.colorPicker.sat;
            this.shaderConfig.colorHSL.lum = this.colorPicker.lum;
            com.serverBound_shaderSet(this.zone.strip.id, this.zone.id, SHADER_SINGLE, this.shaderConfig);
        });
        this.colorPicker = createColorPicker(this.htmlBox.getElementsByClassName("shaderControls")[0], callback);
        callback();
    }

    setZoneTitle(text) {
        let zoneTitle = this.htmlBox.getElementsByClassName("zoneTitle")[0];
        zoneTitle.textContent = text;
        zoneTitle = this.stripZoneListItemBox.getElementsByClassName("zoneTitle")[0];
        zoneTitle.textContent = text;
    }

    syncZoneData() {
        this.zoneSizeTyper.textBox.textContent = this.zone.pixelSize;
    }

    onSizeTyperCheck(newVal) {
        if(newVal.length == 0) newVal = "0";

        let newSize = parseInt(newVal, 10);

        if(1 < newSize) {
            let strip = this.zone.strip;
            let maxAllowed = strip.pixelSize - strip.pixelUsedPos + this.zone.pixelSize;

            if(maxAllowed < newSize) newSize = maxAllowed;
        }
        else newSize = 1;

        return newSize;
    }

    onSizeTyperDone(newVal) {
        this.zone.setPixelSize(newVal);
        com.serverBound_zoneSet(this.zone.strip.id, this.zone.id, this.zone.pixelSize);
        this.renderStrip.checkCanShowCreate();
    }

    onDelete() {
        if(this.deleteBox.classList.contains("buttonDisabled")) return;

        let text = `LED szalag ${this.zone.strip.id} zónáiból\nzóna ${this.zone.id} törlése?\n(${this.zone.pixelSize} pixel)`;
        deleteDialog.show(() => {
            this.deleteBox.onclick = null;
            showHeader(`zóna ${this.zone.strip.id}-${this.zone.id} törölve`, "rgb(190,30,0)", 3000);
            this.renderStrip.deleteRenderZone();
            this.htmlBox.remove();
            this.stripZoneListItemBox.remove();
        }, text);
    }
}

class WriteTyper {
    constructor(parentBox, checkPattern, maxCharLen) {
        this.parentBox = parentBox;
        this.checkPattern = checkPattern;
        this.maxCharLen = maxCharLen;
        this.textBox = parentBox.getElementsByClassName("typerText")[0];
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
        this.buttonSet = parentBox.getElementsByClassName("typerSet")[0];
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

        if(this.onFinalCheck) newVal = this.onFinalCheck(newVal);

        this.textBox.textContent = newVal;

        if(newVal != this.originalValue) {
            if(this.onChanged) this.onChanged(newVal);
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
        this.deleteButton = document.getElementById("deleteDialogConfirm");
        this.htmlBox.onclick = (event) => this.click(event);
    }

    show(deleteTask, text) {
        this.deleteTask = deleteTask;
        this.htmlBox.showModal();
        this.htmlBox.focus();
        this.htmlBox.classList.add("openedDialog");
        let textBox = document.getElementById("deleteDialogText");
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
