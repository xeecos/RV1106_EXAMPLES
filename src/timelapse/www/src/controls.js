import React from "react"
import { Button, Toast, Form, Input } from "antd-mobile";
export default class Controls extends React.Component
{
    constructor()
    {
        super();
        this.state = {
            url:"/preview",
            width:100,
            height:100,
            hbk:200,
            vbk:200,
            exposure:100,
            gain:1000,
            fps:0
        }
    }
    componentDidMount()
    {
        let hbk = Number(localStorage.getItem("hbk")||200);
        let vbk = Number(localStorage.getItem("vbk")||200);
        let exposure = Number(localStorage.getItem("exposure")||100);
        let gain = Number(localStorage.getItem("gain")||1000);
        this.setState({hbk,vbk,exposure,gain,width:window.innerWidth - 10,height:window.innerHeight - 20 - (1080/1920*(window.innerWidth - 10))>>0})
    }
    request(theUrl)
    {
        return new Promise(resolve=>{
            var xmlHttp = new XMLHttpRequest();
            xmlHttp.onreadystatechange = function() { 
                if (xmlHttp.readyState == 4 && xmlHttp.status == 200)
                    resolve(xmlHttp.responseText);
            }
            xmlHttp.open("GET", theUrl, true); // true for asynchronous 
            xmlHttp.send(null);
        })
    }
    render()
    {
        const self = this;
        return <div style={{position:"relative", width:this.state.width, height:this.state.height}}>
            <div style={{position:"absolute",bottom:0,left:0,right:0}}>
                <div style={{position:"absolute",bottom:0,right:0}}>
                    <Button color='primary' fill='outline' onClick={()=>{
                        this.request("/snap")
                    }}>SNAP</Button>
                </div>
                <div style={{position:"absolute",bottom:0,left:0}}>
                    <Button color='primary' fill='outline' onClick={()=>{
                        this.request("/record/start")
                    }}>RECORD START</Button>
                </div>
                <div style={{position:"absolute",bottom:0,left:170}}>
                    <Button color='primary' fill='outline' onClick={()=>{
                        this.request("/record/stop")
                    }}>STOP</Button>
                </div>

            </div>
                <Form layout='horizontal'>
                    <Form.Item
                        label='HBLANK'
                        extra={
                            <div>
                                <Button color='primary' fill='outline' onClick={()=>{
                                    let hbk = this.state.hbk<1?1:this.state.hbk;
                                    this.setState({hbk});
                                    this.request("/hbk/set?v="+hbk);
                                    localStorage.setItem("hbk",hbk);
                                }}>HBLANK</Button>
                            </div>
                        }
                    >
                        <Input ref={(ref)=>{this.hbkRef = ref;}} type="number" placeholder='请输入HBLANK' value={this.state.hbk} onChange={(v)=>{
                            let hbk = Number(v);
                            hbk = hbk<1?1:hbk;
                            this.setState({hbk});
                        }} onEnterPress={()=>{
                            let hbk = this.state.hbk<1?1:this.state.hbk;
                            this.setState({hbk});
                            this.request("/hbk/set?v="+hbk);
                            localStorage.setItem("hbk",hbk);
                            this.hbkRef.blur();
                        }} clearable />
                    </Form.Item>
                </Form>
            <div>
                <Form layout='horizontal'>
                    <Form.Item
                        label='VBLANK'
                        extra={
                            <div>
                                <Button color='primary' fill='outline' onClick={()=>{
                                    let vbk = this.state.vbk<1?1:this.state.vbk;
                                    this.setState({vbk});
                                    this.request("/vbk/set?v="+vbk);
                                    localStorage.setItem("vbk",vbk);
                                }}>VBLANK</Button>
                            </div>
                        }
                    >
                        <Input ref={(ref)=>{this.vbkRef = ref;}} type="number" placeholder='请输入VBLANK' value={this.state.vbk} onChange={(v)=>{
                            let vbk = Number(v);
                            vbk = vbk<1?1:vbk;
                            this.setState({vbk});
                        }} onEnterPress={()=>{
                            let vbk = this.state.vbk<1?1:this.state.vbk;
                            this.setState({vbk});
                            this.request("/vbk/set?v="+vbk);
                            localStorage.setItem("vbk",vbk);
                            this.vbkRef.blur();
                        }} clearable />
                    </Form.Item>
                </Form>
            </div>
            <div>
                <Form layout='horizontal'>
                    <Form.Item
                        label='EXPOSURE'
                        extra={
                            <div>
                                <Button color='primary' fill='outline' onClick={()=>{
                                    let exposure = this.state.exposure<1?1:this.state.exposure;
                                    this.setState({exposure});
                                    this.request("/exposure/set?v="+exposure);
                                    localStorage.setItem("exposure",exposure);
                                }}>EXPOSURE</Button>
                            </div>
                        }
                    >
                        <Input ref={(ref)=>{this.exposureRef = ref;}} type="number" placeholder='请输入EXPOSURE' value={this.state.exposure} onChange={(v)=>{
                            let exposure = Number(v);
                            exposure = exposure<1?1:exposure;
                            this.setState({exposure});
                        }} onEnterPress={()=>{
                            let exposure = this.state.exposure<1?1:this.state.exposure;
                            this.setState({exposure});
                            this.request("/exposure/set?v="+exposure);
                            localStorage.setItem("exposure",exposure);
                            this.exposureRef.blur();
                        }} clearable />
                    </Form.Item>
                </Form>
            </div>
            <div>
                <Form layout='horizontal'>
                    <Form.Item
                        label='GAIN'
                        extra={
                            <div>
                                <Button color='primary' fill='outline' onClick={()=>{
                                    let gain = this.state.gain<1?1:this.state.gain;
                                    this.setState({gain});
                                    this.request("/gain/set?v="+gain);
                                    localStorage.setItem("gain",gain);
                                }}>GAIN</Button>
                            </div>
                        }
                    >
                        <Input ref={(ref)=>{this.gainRef = ref;}} type="number" placeholder='请输入GAIN' value={this.state.gain} onChange={(v)=>{
                            let gain = Number(v);
                            this.setState({gain});
                        }} onEnterPress={()=>{
                            let gain = this.state.gain<1?1:this.state.gain;
                            this.setState({gain});
                            this.request("/gain/set?v="+gain);
                            localStorage.setItem("gain",gain);
                            this.gainRef.blur();
                        }} clearable />
                    </Form.Item>
                </Form>
            </div>

            <div>
                <Form layout='horizontal'>
                    <Form.Item
                        label='FPS'
                        extra={
                            <div>
                                <Button color='primary' fill='outline' onClick={()=>{
                                    this.request("/fps").then(res=>{
                                        this.setState({fps:JSON.parse(res).fps})
                                        // Toast.show({
                                        //     icon: 'success',
                                        //     content: 'fps:'+JSON.parse(res).fps,
                                        // })
                                    })
                                }}>FPS</Button>
                            </div>
                        }
                    >
                        <Input type="number" placeholder='' value={this.state.fps} clearable />
                    </Form.Item>
                </Form>
            </div>
        </div>
    }
}