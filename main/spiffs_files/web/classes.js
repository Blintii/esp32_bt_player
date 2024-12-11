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
        this.stripSizeTyper = new WriteTyper(htmlBox, /[^0-9]+/, newVal => {
            this.strip.pixelSize = parseInt(newVal, 10);
            com.serverBound_setAddress(this.strip.id, this.strip.pixelSize);
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

    constructor(htmlBox, checkPattern, onDone) {
        this.checkPattern = checkPattern;
        this.onDone = onDone;
        this.textBox = htmlBox.getElementsByClassName("uiWriteTyperText")[0];
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
        this.buttonSet = htmlBox.getElementsByClassName("uiWriteTyperSet")[0];
        this.parentTextBox = htmlBox.getElementsByClassName("uiWriteTyperBox")[0];
        this.parentTextBox.onmousedown = (event) => {
            if(this.typing) event.preventDefault();
        };
        this.parentTextBox.onclick = (event) => {
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

        if(s.length > 3) s = s.substring(1, 4);

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
