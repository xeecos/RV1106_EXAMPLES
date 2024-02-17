const path = require("path");
const os = require("os");
const webpack = require("webpack");
const HtmlWebpackPlugin = require("html-webpack-plugin");
const BomPlugin = require("webpack-utf8-bom");
const CopyWebpackPlugin = require("copy-webpack-plugin");
module.exports = [
  {
    mode: "development",
    // devtool: "cheap-module-eval-source-map",
    entry: ["@babel/polyfill", "./src/app.js"],
    output: {
      path: path.resolve(__dirname, "./dist"),
      filename: "app.bundle.js",
      sourceMapFilename: "bundle.map",
    },
    externals: {},
    resolve: {
      extensions: [".js", ".jsx"],
    },
    // externals: {
    //   sharp: "commonjs sharp",
    // },
    module: {
      rules: [
        {
          test: /\.(js|jsx)$/,
          loader: "babel-loader",
          include: [path.resolve(__dirname, "src")],
          exclude: [/node_modules/],
          options: {
            presets: ["@babel/env", "@babel/react"],
          },
        },
        {
          test: /\.(png|jpg|gif|svg|wasm)$/,
          loader: "file-loader",
          options: {
            name: "assets/[name].[ext]?[hash]",
          },
        },
        {
          test: /\.(woff|woff2|ttf|svg|eot)$/,
          use: {
            loader: "url-loader",
            options: {
              limit: 10000,
              name: "font/[name].[ext]?[hash]",
            },
          },
        },
        {
          test: /\.(png)$/,
          loader: "url-loader",
          options: {
            name: "assets/[name].[ext]?[hash]",
          },
        },
        {
          test: /\.css$/,
          use: [
            {
              loader: "style-loader",
            },
            {
              loader: "css-loader",
            },
          ],
        },
        {
          test: /\.(scss|sass)$/,
          use: [
            {
              loader: "style-loader",
            },
            {
              loader: "css-loader",
              options: {
                esModule: false,
              },
            },
            {
              loader: "sass-loader",
              options: {
                esModule: false,
              },
            },
          ],
        },
        {
          test: /\.(htm|html)$/,
          loader: "html-loader",
          options: {
            esModule: false,
          },
        },
      ],
    },
    // externals: {
    //     'sharp': 'commonjs sharp'
    // },
    target: "web",
    plugins: [
      new webpack.DefinePlugin({
        "process.browser": "true",
      }),
      new HtmlWebpackPlugin({
        template: "src/html/index.html",
        filename: "index.html",
        inject: true,
      }),
      new BomPlugin(true, /\.(js|jsx)$/),
      new CopyWebpackPlugin({
        patterns: [
          {
            from: path.resolve("src", "assets"),
            to: path.resolve("dist", "assets"),
          },
          {
            from: path.resolve("src", "locales"),
            to: path.resolve("dist", "locales"),
          },
        ],
      }),
    ],
  }
];