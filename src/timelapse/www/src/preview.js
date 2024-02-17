import React from "react"
import { Image } from "antd-mobile";
export default class Preview extends React.Component
{
    constructor()
    {
        super();
        this.state = {
            url:"/preview",
            width:100,
            height:100
        }
    }
    componentDidMount()
    {
        this.setState({width:window.innerWidth - 10,height:(1080/1920*(window.innerWidth - 10))>>0});
    }
    render()
    {
        const self = this;
        return <div style={{position:"relative"}}><Image width={this.state.width} height={this.state.height} src={this.state.url} onClick={()=>{
            self.setState({url:`/preview?time=${Date.now()}`})
        }}></Image></div>
    }
}