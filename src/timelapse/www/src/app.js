import { Button, Image, Toast } from 'antd-mobile'
import React from "react"
import Preview from "./preview"
import ReactDOM from "react-dom/client"
import "./style/style.css"
import Controls from './controls'
document.documentElement.setAttribute(
    'data-prefers-color-scheme',
    'dark'
)
const root = ReactDOM.createRoot(
    document.getElementById('app')
);
const element = <div style={{ margin: "10px 5px", width:"100%", top:0, bottom: 0, position:"absolute"}}>
    <Preview />
    <Controls />
</div>;
root.render(element);