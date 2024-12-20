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
        console.log("Websocket rx:", event.data);
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
        throw new Error(`nincs lekezelve csoro CID1: ${u8Array}`);
    }

    // CID 2
    clientBound_shaderConfig(u8Array) {
        throw new Error(`nincs lekezelve csoro CID2: ${u8Array}`);
    }

    tx(packet) {
        try {
            console.log(`kűdés van ${new Uint8Array(packet)}`);
            ws.ws.send(packet);
        }
        catch(e) {
            console.error(e);
        }
    }

    // SID 0
    serverBound_stripSet(stripIndex, pixelSize, rgbOrder) {
        /* SID + stripIndex + pixelSize (uint32) + rgbOrder (3 char) */
        let len = 1 + 1 + 4 + 3;
        let buf = new ArrayBuffer(len);
        let dataView = new DataView(buf);
        dataView.setUint8(0, 0);
        dataView.setUint8(1, stripIndex);
        dataView.setUint32(2, pixelSize);
        new Uint8Array(buf, 6, 3).set(new TextEncoder().encode(rgbOrder));
        this.tx(buf);
    }

    // SID 1
    serverBound_zoneSet(stripIndex, zoneIndex, pixelSize) {
        /* SID + stripIndex + zoneIndex + pixelSize (uint32) */
        let len = 1 + 1 + 1 + 4;
        let buf = new ArrayBuffer(len);
        let dataView = new DataView(buf);
        dataView.setUint8(0, 1);
        dataView.setUint8(1, stripIndex);
        dataView.setUint8(2, zoneIndex);
        dataView.setUint32(3, pixelSize);
        this.tx(buf);
    }

    // SID 2
    serverBound_shaderSet(stripIndex, zoneIndex, shaderType, shaderConfig) {
        /* SID + stripIndex + zoneIndex + shaderType */
        let len = 1 + 1 + 1 + 1;

        switch(shaderType) {
            case SHADER_SINGLE: len += 3*4; break;
            default: throw new Error(`nincs lekezelve csoro shader: ${shaderType}`);
        }

        let buf = new ArrayBuffer(len);
        let dataView = new DataView(buf);
        dataView.setUint8(0, 2);
        dataView.setUint8(1, stripIndex);
        dataView.setUint8(2, zoneIndex);
        dataView.setUint8(3, shaderType);

        switch(shaderType) {
            case SHADER_SINGLE:
                dataView.setFloat32(4, shaderConfig.colorHSL.hue);
                dataView.setFloat32(8, shaderConfig.colorHSL.sat);
                dataView.setFloat32(12, shaderConfig.colorHSL.lum);
                break;
        }

        this.tx(buf);
    }
}
