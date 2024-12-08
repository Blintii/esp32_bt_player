class WebSocketHandler {
    newSocket() {
        this.ws = new WebSocket("/ws");
        // this.ws.onopen = () => this.handleOpen();
        this.ws.onclose = () => this.handleClose();
        this.ws.onerror = (event) => this.handleError(event);
        this.ws.onmessage = (event) => this.handleMessage(event);
    }

    handleClose() {
        console.error("WebSocket closed");
        showHeader("OFFLINE", "rgb(190,30,0)");
        setTimeout(() => this.newSocket(), 1000);
    }

    handleError(event) {
        console.log("WebSocket error event:");
        console.log(event);
    }

    async handleMessage(event) {
        hideHeader();
        let binaryBuf = await event.data.arrayBuffer();
        com.rx(new Uint8Array(binaryBuf));
    }
}

class MessageHandler {
    constructor() {
        this.isFirst = true;
    }

    rx(u8Array) {
        // 1st byte: clientbound ID
        switch(u8Array[0]) {
            case 0:
                this.clientBound_stripConfig(u8Array.slice(1));
                break;
            case 1:
                this.clientBound_zoneConfig(u8Array.slice(1));
                break;
            case 2:
                this.clientBound_shaderConfig(u8Array.slice(1));
                break;
            default: console.error(`unknown CID: ${u8Array[0]}`);
        }
    }

    // CID 0
    clientBound_stripConfig(u8Array) {
        refreshStripConfig(this.isFirst, u8Array);
        if(this.isFirst) this.isFirst = false;
    }

    // CID 1
    clientBound_zoneConfig(u8Array) {
        console.log(`clientBound_zoneConfig: nincs lekezelve csoro CID1: ${u8Array}`)
    }

    // CID 2
    clientBound_shaderConfig(u8Array) {
        console.log(`clientBound_shaderConfig: nincs lekezelve csoro CID2: ${u8Array}`)
    }

    tx(serverBound_ID, dataArray) {
        try {
            let packet = [serverBound_ID].concat(dataArray);
            ws.ws.send(new Uint8Array(packet));
        }
        catch(e) {
            console.error(e);
        }
    }

    // SID 0
    serverBound_coilSetOn(stripIndex, coilN) {
        this.tx(0, [stripIndex, coilN]);
    }

    // SID 1
    serverBound_coilSetOff(stripIndex, coilN) {
        this.tx(1, [stripIndex, coilN]);
    }

    // SID 2
    serverBound_setAddress(stripIndex, busAddress) {
        this.tx(2, [stripIndex, busAddress]);
    }

    // SID 3
    serverBound_deleteDevice(stripIndex) {
        this.tx(3, [stripIndex]);
    }
}
