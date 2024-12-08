class Strip {
    constructor(id, pixelSize, rgbOrder) {
        this.id = id;
        this.exist = false;
        this.pixelSize = pixelSize;
        this.rgbOrder = rgbOrder;
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
        const title = this.htmlBox.getElementsByClassName("uiHeader")[0];
        const deviceName = title.getElementsByClassName("deviceName")[0];
        deviceName.textContent = text;
    }

    setupControls() {
        this.controlsUi = new RenderControlsUI(this);
        this.controlsUi.setupUI();
        this.deleteBox.onclick = () => {
            let text = `Delete strip ${this.strip.id}?\n(${this.strip.pixelSize} pixel)`;
            deleteDialog.show(() => this.deleteControls(), text);
        };
    }

    syncStripData() {
        this.stripSizeTyper.stripSizeBox.textContent = this.strip.pixelSize;
    }

    deleteControls() {
        this.controlsUi.controlBox.innerHTML = "";
        this.stripSizeTyper.stripSizeBox.textContent = "";
        this.deleteBox.onclick = null;
        this.controlsUi = null;

        if(this.strip.exist) {
            showHeader(`Device ${this.strip.id} deleted`, "rgb(190,30,0)", 3000);
            this.strip.pixelSize = null;
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
        const bodyBox = this.htmlBox.getElementsByClassName("uiBody")[0];
    }

    checkBoxCallback(box) {
        if(box.classList.contains("deniedCheckBox")) return;

        if(box.classList.contains("checked")) {

        }
        else {

        }
    }
}

class StripSizeTyper {
    #hexPattern = /[^0-9]+/;
    #startZerosPattern = /\b(0(?!\b))+/g;

    constructor(renderStrip, htmlBox) {
        this.parent = renderStrip;
        this.stripSizeBox = htmlBox.getElementsByClassName("uiStripSizeText")[0];
        this.parentstripSizeBox = htmlBox.getElementsByClassName("uiStripSizeBox")[0];
        this.buttonSizeSet = htmlBox.getElementsByClassName("uiStripSizeSet")[0];
        this.stripSizeBox.oninput = (event) => this.typeCallback(event);
        this.stripSizeBox.onblur = () => this.endType();
        this.stripSizeBox.onpaste = (event) => event.preventDefault();
        this.parentstripSizeBox.onmousedown = (event) => {
            if(this.typing) event.preventDefault();
        };
        this.parentstripSizeBox.onclick = (event) => {
            if(this.typing) {
                // if edit button clicked -> force trigger focus out (blur) event
                if(this.buttonSizeSet.contains(event.target)) {
                    document.activeElement.blur();
                }
            }
            else this.startType();
        };
    }

    startType() {
        this.typing = true;
        this.originalValue = this.stripSizeBox.textContent;
        this.stripSizeBox.focus();
        this.typeCallback();
    }

    endType() {
        this.typing = false;
        let newVal = this.stripSizeBox.textContent;

        if(newVal != this.originalValue) {
            if(newVal.length == 0) newVal = "0";

            this.stripSizeBox.textContent = newVal.replace(this.#startZerosPattern, '');
            this.parent.strip.pixelSize = newVal;
            com.serverBound_setAddress(this.parent.strip.id, parseInt(newVal, 16));
        }
    }

    typeCallback(event) {
        document.getSelection()
        this.check();
        let sel = document.getSelection();
        sel.modify("move", "forward", "lineboundary");

        if(event) {
            // detect ENTER pressed
            if(event.inputType == "insertParagraph" && event.data == null) {
                document.activeElement.blur();
            }
        }
    }

    check() {
        let s = this.stripSizeBox.textContent;
        s = s.replace(this.#hexPattern, "");

        if(s.length > 3) s = s.substring(1, 4);

        this.stripSizeBox.textContent = s.toUpperCase();
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
