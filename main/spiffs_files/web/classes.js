class Strip {
    constructor(id, pixelSize, rgbOrder) {
        this.id = id;
        this.exist = false;
        this.pixelSize = pixelSize;
        this.rgbOrder = rgbOrder
        this.inputs = [];
        this.coils = [];
        this.address = null;

        for(let i = 0; i < 8; i++) {
            this.inputs[i] = false;
            this.coils[i] = false;
        }
    }

    setInputValue(index, val) {
        this.inputs[index] = val;
    }

    setCoilValue(index, val) {
        this.coils[index] = val;
    }
}

class RenderStrip {
    constructor(strip, htmlBox) {
        this.strip = strip;
        this.htmlBox = htmlBox;
        this.deleteBox = htmlBox.getElementsByClassName("uiDeviceDelete")[0];
        this.stripSizeTyper = new StripSizeTyper(this, htmlBox);
    }

    setDeviceName(text) {
        const title = this.htmlBox.getElementsByClassName("titleBox")[0];
        const deviceName = title.getElementsByClassName("deviceName")[0];
        deviceName.textContent = text;
    }

    setupControls() {
        this.controlsUi = new RenderControlsUI(this);
        this.controlsUi.setupUI();
        this.deleteBox.onclick = () => {
            let deviceID = this.strip.id;
            let addressText = this.strip.address;
            let text = `Delete device ${deviceID}?\n(address: 0x${addressText})`;
            deleteDialog.show(() => this.deleteControls(), text);
        };
        this.deleteBox.classList.remove("deviceNotExist");
    }

    syncStripData() {
        this.stripSizeTyper.addressBox.textContent = this.strip.address;
        this.controlsUi.syncControls();
    }

    deleteControls() {
        this.deleteBox.classList.add("deviceNotExist");
        this.controlsUi.controlBox.innerHTML = "";
        this.stripSizeTyper.addressBox.textContent = "";
        this.deleteBox.onclick = null;
        this.controlsUi = null;

        if(this.strip.exist) {
            showHeader(`Device ${this.strip.id} deleted`, "rgb(190,30,0)", 3000);
            this.strip.address = null;
            this.strip.exist = false;
            com.serverBound_deleteDevice(this.strip.id);
        }
    }
}

class RenderControlsUI {
    constructor(renderStrip) {
        this.parent = renderStrip;
        this.strip = renderStrip.strip;
        this.htmlBox = renderStrip.htmlBox;
        this.controlBox = this.htmlBox.getElementsByClassName("uiControls")[0];
    }

    setupUI() {
        const coilBox = tmpControlItem.content.cloneNode(true).firstElementChild;
        const inputBox = tmpControlItem.content.cloneNode(true).firstElementChild;
        this.controlBox.appendChild(coilBox);
        this.controlBox.appendChild(inputBox);

        this.addCoilsUI(coilBox);
        this.addInputsUI(inputBox);
    }

    addCoilsUI(coilsBox) {
        const coilsBoxText = coilsBox.getElementsByClassName("uiHeader")[0];
        coilsBoxText.textContent = "Coils";
        const bodyBox = coilsBox.getElementsByClassName("uiBody")[0];

        for(let i = 0; i < this.strip.pixelSize; i++) {
            let clone = tmpCheckBox.content.cloneNode(true).firstElementChild;
            clone.onclick = () => this.checkBoxCallback(clone, i);
            let text = clone.getElementsByClassName("uiCheckBoxText")[0];
            text.textContent = i;
            bodyBox.appendChild(clone);
        }
    }

    addInputsUI(inputsBox) {
        const inputsBoxText = inputsBox.getElementsByClassName("uiHeader")[0];
        inputsBoxText.textContent = "Inputs";
        const bodyBox = inputsBox.getElementsByClassName("uiBody")[0];

        for(let i = 0; i < 8; i++) {
            let clone = tmpLED.content.cloneNode(true).firstElementChild;
            let text = clone.getElementsByClassName("ledText")[0];
            text.textContent = i;
            bodyBox.appendChild(clone);
        }
    }

    checkBoxCallback(box, index) {
        if(box.classList.contains("deniedCheckBox")) return;

        if(box.classList.contains("checked")) {
            this.strip.setCoilValue(index, false);
            this.setCoilChecked(box, false);
            com.serverBound_coilSetOff(this.strip.id, index);
        }
        else {
            this.strip.setCoilValue(index, true);
            this.setCoilChecked(box, true);
            com.serverBound_coilSetOn(this.strip.id, index);
        }
    }

    syncControls() {
        const checkBoxes = this.controlBox.getElementsByClassName("uiCheckBox");
        const leds = this.controlBox.getElementsByClassName("LED");

        for(let i = 0; i < this.strip.pixelSize; i++) {
            this.setCoilChecked(checkBoxes[i], this.strip.coils[i]);
            this.setInputOn(leds[i], this.strip.inputs[i]);
        }
    }

    setCoilChecked(box, val) {
        if(true == val) box.classList.add("checked");
        else box.classList.remove("checked");
    }

    setInputOn(box, val) {
        if(true == val) box.classList.add("on");
        else box.classList.remove("on");
    }
}

class StripSizeTyper {
    #hexPattern = /[^0-9]+/;

    constructor(renderStrip, htmlBox) {
        this.parent = renderStrip;
        this.addressBox = htmlBox.getElementsByClassName("uiDeviceAddressText")[0];
        this.parentAddressBox = htmlBox.getElementsByClassName("uiDeviceAddressBox")[0];
        this.buttonAddressSet = htmlBox.getElementsByClassName("uiDeviceAddressSet")[0];
        this.addressBox.oninput = (event) => this.typeCallback(event);
        this.addressBox.onblur = () => this.endType();
        this.addressBox.onpaste = (event) => event.preventDefault();
        this.parentAddressBox.onmousedown = (event) => {
            if(this.typing) event.preventDefault();
        };
        this.parentAddressBox.onclick = (event) => {
            if(this.typing) {
                // if edit button clicked -> force trigger focus out (blur) event
                if(this.buttonAddressSet.contains(event.target)) {
                    document.activeElement.blur();
                }
            }
            else this.startType();
        };
    }

    startType() {
        this.typing = true;
        this.originalValue = this.addressBox.textContent;
        this.addressBox.focus();
        this.typeCallback();
    }

    endType() {
        this.typing = false;
        let newVal = this.addressBox.textContent;

        if(newVal != this.originalValue) {
            if(newVal.length == 0) newVal = "00";
            else if(newVal.length == 1) newVal = "0" + newVal;

            this.addressBox.textContent = newVal;
            this.parent.strip.address = newVal;
            com.serverBound_setAddress(this.parent.strip.id, parseInt(newVal, 16));
        }
    }

    typeCallback(event) {
        document.getSelection()
        this.check();
        let sel = document.getSelection();
        sel.modify("move", "forward", "paragraphboundary");

        if(event) {
            // detect ENTER pressed
            if(event.inputType == "insertParagraph" && event.data == null) {
                document.activeElement.blur();
            }
        }
    }

    check() {
        let s = this.addressBox.textContent;
        s = s.replace(this.#hexPattern, "");

        if(s.length > 2) s = s.substring(1, 3);

        this.addressBox.textContent = s.toUpperCase();
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
