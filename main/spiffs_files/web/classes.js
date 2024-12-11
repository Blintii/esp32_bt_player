class Strip {
    constructor(id, pixelSize, rgbOrder) {
        this.id = id;
        this.pixelSize = pixelSize;
        this.rgbOrder = rgbOrder;
        this.zones = []
    }
}

class Zone {
    constructor(strip) {
        this.strip = strip
    }
}

class RenderStrip {
    constructor(strip, htmlBox) {
        this.strip = strip;
        this.htmlBox = htmlBox;
        this.deleteBox = htmlBox.getElementsByClassName("uiZoneDelete")[0];
        let writeTyperBox = htmlBox.getElementsByClassName("stripSizeBox")[0];
        this.stripSizeTyper = new WriteTyper(writeTyperBox, /[^0-9]+/, 3, newVal => {
            this.strip.pixelSize = parseInt(newVal, 10);
            com.serverBound_stripSet(this.strip.id, this.strip.pixelSize, this.strip.rgbOrder);
        });
        let rgbOrderBox = htmlBox.getElementsByClassName("RGBOrderBox")[0];
        this.rgbOrderTyper = new WriteTyper(rgbOrderBox, /[^RGBrgb]+/, 3, newVal => {
            if(!(newVal.includes('R') && newVal.includes('G') && newVal.includes('B'))) {
                newVal = "RGB";
                this.rgbOrderTyper.textBox.textContent = newVal;
            }

            if(newVal != this.strip.rgbOrder) {
                this.strip.rgbOrder = newVal
                com.serverBound_stripSet(this.strip.id, this.strip.pixelSize, this.strip.rgbOrder);
            }
        });
    }

    setStripTitle(text) {
        const title = this.htmlBox.getElementsByClassName("uiHeader")[0];
        const stripTitle = title.getElementsByClassName("stripTitle")[0];
        stripTitle.textContent = text;
    }

    setupControls() {
        this.controlsUi = new RenderControls(this);
        this.controlsUi.setupUI();

        if(this.deleteBox) {
            this.deleteBox.onclick = () => {
                let text = `Delete zone ${this.strip.id}?\n(${this.strip.pixelSize} pixel)`;
                deleteDialog.show(() => this.deleteControls(), text);
            };
        }
    }

    syncStripData() {
        this.stripSizeTyper.textBox.textContent = this.strip.pixelSize;
        this.rgbOrderTyper.textBox.textContent = this.strip.rgbOrder;
    }

    deleteControls() {
        this.controlsUi.controlBox.innerHTML = "";
        this.stripSizeTyper.textBox.textContent = "";
        this.deleteBox.onclick = null;
        this.controlsUi = null;

        showHeader(`Strip ${this.strip.id} deleted`, "rgb(190,30,0)", 3000);
        this.strip.pixelSize = 0;
        com.serverBound_deleteDevice(this.strip.id);
    }
}

class RenderControls {
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

        if(newVal != this.originalValue) this.onDone(newVal);
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
