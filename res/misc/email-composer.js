/*
 * Copyright (c) 2009-2015 Samsung Electronics Co., Ltd All Rights Reserved
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//console.log("==> screen.width:" + screen.width);
//console.log("==> window.innerWidth:" + window.innerWidth);
//console.log("==> window.devicePixelRatio:" + window.devicePixelRatio);

var Direction = {
		"LEFT"			: 0,
		"RIGHT"			: 1,
		"TOP"			: 2,
		"BOTTOM"		: 3,
		"LEFT_TOP"		: 4,
		"RIGHT_TOP"		: 5,
		"LEFT_BOTTOM"	: 6,
		"RIGHT_BOTTOM"	: 7,
		"MOVE"			: 8
};

var FontSize = {
		"1" 			: 10,
		"2" 			: 14,
		"3" 			: 16,
		"4" 			: 18,
		"5" 			: 24,
		"6" 			: 32,
		"7" 			: 48
};

/* Define names */
var G_NAME_NEW_MESSAGE = 'DIV_NewMessage';
var G_NAME_ORG_MESSAGE = 'DIV_OrgMessage';
var G_NAME_ORG_MESSAGE_BAR = 'DIV_OrgMessageBar';
var G_NAME_ORG_MESSAGE_BAR_CHECKBOX = 'DIV_OrgMessageBarCheckbox';
var G_NAME_ORG_MESSAGE_BAR_TEXT = "DIV_OrgMessageBarText";
var G_NAME_ORG_MESSAGE_BAR_ROW = "DIV_OrgMessageBarRow";

var G_NAME_CSS_ORG_MESSAGE_BAR_CHECKED = "css-originalMessageBar-checked";
var G_NAME_CSS_ORG_MESSAGE_BAR_UNCHECKED = "css-originalMessageBar-unchecked";
var G_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_CHECKED = "css-originalMessageBarCheckbox-checked";
var G_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_UNCHECKED = "css-originalMessageBarCheckbox-unchecked";

var G_VAL_IMAGE_RESIZE_RATIO = 0.95;
var G_VAL_PIXEL_RATIO = window.devicePixelRatio; // 1.5 in kiran
var G_VAL_DOW_W = 19 / G_VAL_PIXEL_RATIO;
var G_VAL_DOW_H = 19 / G_VAL_PIXEL_RATIO;
var G_VAL_REMOVE_BTN_WH = 54 / G_VAL_PIXEL_RATIO;
var G_VAL_LAYER_BORDER = 3 / G_VAL_PIXEL_RATIO;
var G_VAL_TOUCH_SPACE = 30 / G_VAL_PIXEL_RATIO;
var G_VAL_TOUCH_SPACE_DIFF = G_VAL_TOUCH_SPACE * 2;

var G_VAL_DEFAULT_FONT_COLOR = "rgb(0, 0, 0)";
var G_VAL_DEFAULT_BG_COLOR = "rgb(255, 255, 255)";
var G_VAL_DEFAULT_FONT_SIZE = "16px";
var G_VAL_TEXT_NODE_TYPE = 3;
var G_VAL_ORG_MESSAGE_BAR_TEXT_ALIGN = "display:table-cell; vertical-align: middle; padding-left:10px; padding-top:10px; padding-bottom:10px; width:auto;";
var G_VAL_ORG_MESSAGE_BAR_TEXT_SIZE = 18;

var G_VAL_EDITOR_MARGIN_SIZE = 10;
var G_VAL_IMG_STYLE_MARGIN = "8px 0px";

var g_whileResizing = false;
var g_dragData = null;
var g_selectedImg = null;
var g_removeBtn = null;
var g_caretPosition = null;

var g_isTouchStartOnOriginalMessageBar = false;
var g_isTouchMoveOnOriginalMessageBar = false;

var g_isTouchStartOnImage = false;
var g_isTouchMoveOnImage = false;

var g_selectionRange = null;

var g_curFontSize = 0;
var g_curBold = 0;
var g_curUnderline = 0;
var g_curItalic = 0;
var g_curFontColor = "";
var g_curBgColor = "";
var g_curOrderedList = 0;
var g_curUnorderedList = 0;

var g_curEventStr = "";
var g_curEventIsGroup = false

var g_lastRange = null;
var g_lastRangeIsForward = false;

var g_removeInvalidImages = false;

function CaretPos(x, top, bottom, isCollapsed) {
	this.x = Math.round(x);
	this.top = Math.round(top);
	this.bottom = Math.round(bottom);
	this.isCollapsed = isCollapsed;

	this.toString = function() {
		return this.x + " " + this.top + " " + this.bottom + " " + this.isCollapsed;
	};
}

function SendUserEvent(event) {
	if (g_curEventIsGroup) {
		g_curEventStr = g_curEventStr + ";" + event;
	} else if (g_curEventStr.length > 0) {
		g_curEventStr = "group://events?" + g_curEventStr + ";" + event;
		g_curEventIsGroup = true;
	} else {
		g_curEventStr = event.toString();
		setTimeout(DispatchUserEvents, 0);
	}
}

function DispatchUserEvents() {
	window.location = g_curEventStr;
	g_curEventStr = "";
	g_curEventIsGroup = false;
}

/* Related to the behavior of Original Message Bar */
function OnTouchStart_OriginalMessageBar(ev) {
	console.log(arguments.callee.name + "()");
	g_isTouchStartOnOriginalMessageBar = true;
}

function OnTouchMove_OriginalMessageBar(ev) {
	console.log(arguments.callee.name + "()");
	if (!g_isTouchStartOnOriginalMessageBar) {
		return;
	}
	g_isTouchMoveOnOriginalMessageBar = true;
}

function OnTouchEnd_OriginalMessageBar(ev) {
	console.log(arguments.callee.name + "()");
	if (g_isTouchStartOnOriginalMessageBar && !g_isTouchMoveOnOriginalMessageBar) {
		console.log("clicked!");
		ev.stopPropagation();
		ev.preventDefault();

		SwitchOriginalMessageState();
	}
	g_isTouchStartOnOriginalMessageBar = false;
	g_isTouchMoveOnOriginalMessageBar = false;
}

function OnkeyDown_Checkbox(obj, event) {
	console.log(arguments.callee.name + "()");
	if (event.keyCode == 13) { // Enter key
		console.log(arguments.callee.name + ": [enter]");
		event.preventDefault();
		SwitchOriginalMessageState();
	} else if (event.keyCode == 32) { // Spacebar
		console.log(arguments.callee.name + ": [spacebar]");
		event.preventDefault();
		SwitchOriginalMessageState();
	} else {
		console.log(arguments.callee.name + ": [" + event + " / " + event.keyCode + "]");
	}
}

function SwitchOriginalMessageState()
{
	console.log(arguments.callee.name + "()");
	var originalMessage = document.getElementById(G_NAME_ORG_MESSAGE);
	var originalMessageBar = document.getElementById(G_NAME_ORG_MESSAGE_BAR);
	var originalMessageBarCheckbox = document.getElementById(G_NAME_ORG_MESSAGE_BAR_CHECKBOX);

	if (originalMessageBarCheckbox.className == G_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_CHECKED) {
		originalMessageBarCheckbox.className = G_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_UNCHECKED;
		originalMessageBar.className = G_NAME_CSS_ORG_MESSAGE_BAR_UNCHECKED;
		SendUserEvent("clickcheckbox://0");
		originalMessage.style.display = "none";
	} else {
		originalMessageBarCheckbox.className = G_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_CHECKED;
		originalMessageBar.className = G_NAME_CSS_ORG_MESSAGE_BAR_CHECKED;
		SendUserEvent("clickcheckbox://1");
		originalMessage.style.display = "block";
	}
}

/* Related to the behavior of focus */
function OnFocus_NewMessageDIV(ev) {
	console.log(arguments.callee.name + "()");
	SendUserEvent("getfocusnewmessagediv://");
}

function OnFocus_OrgMessageDIV(ev) {
	console.log(arguments.callee.name + "()");
	SendUserEvent("getfocusorgmessagediv://");
}

/* Related to the behavior of etc... */
function OnNodeInserted(obj, event) {
	//console.log(arguments.callee.name + "()");
	if (event.target.tagName === "DIV") {
		event.target.setAttribute('dir', 'auto');
	}
}

function OnKeyDown(obj, event) {
	//console.log(arguments.callee.name + "()");
	if (event.keyCode == 9) { // Tab Key
		console.log(arguments.callee.name + ": [tab]");
		event.preventDefault();
		document.execCommand('insertHTML', false, "\u00A0\u00A0\u00A0\u00A0");
	} else if (event.ctrlKey && (event.keyCode == 89)) { // 'ctrl+y' Key
		console.log(arguments.callee.name + ": [ctrl+y]");
		document.execCommand('Redo', false, null);
	} else if (event.ctrlKey && (event.keyCode == 90)) { // 'ctrl+z' Key
		console.log(arguments.callee.name + ": [ctrl+z]");
		document.execCommand('Undo', false, null);
	} else if ((event.keyCode == 13) || (event.keyCode == 8)) { // Enter, Backspace
		console.log(arguments.callee.name + ": [special key]");
		setTimeout(NotifyCaretPosChange, 0);
	} else {
		console.log(arguments.callee.name + ": [" + event + " / " + event.keyCode + "]");
	}
}

function NotifyCaretPosChange() {
	var caretPos = GetCaretPosition();
	if (caretPos) {
		SendUserEvent("caretposchanged://" + caretPos);
	}
}

/* Related to the behavior of Image conrol */
function OnPaste(obj, ev) {
	console.log(arguments.callee.name + "()");
	var imageUrlNo = -1;
	if (ev.clipboardData.types) {
		var i = 0;
		while (i < ev.clipboardData.types.length) {
			var key = ev.clipboardData.types[i];
			//var val = ev.clipboardData.getData(key);
			//console.log((i + 1) + ': ' + key + ' - ' + val);

			if (key == "Image") {
				imageUrlNo = i;
				break;
			}
			i++;
		}
	}
	//console.log("Image No: " + imageUrlNo);

	if (imageUrlNo == -1) {
		// P140612-05478: Due to performance issue on webkit pasting and rendering a large amount of text, we need to set the limit of the text will be pasted. (100K on GalaxyS5)
		var elements = document.getElementsByTagName("body");
		var bodyLength = elements[0].innerHTML.length;
		var pasteLength = ev.clipboardData.getData(ev.clipboardData.types[0]).length;
		console.log("Length: body:(" + bodyLength + "), paste:(" + pasteLength + ")");

		if ((bodyLength + pasteLength) >= 102400) {
			console.log("Exceeded 100K!!");
			SendUserEvent("exceededmax://");
			ev.preventDefault();
			ev.stopPropagation();
		}
		return;
	}
}

function InsertNodeAtCursor(node) {
	console.log(arguments.callee.name + "()");
	var sel, range, html;

	sel = window.getSelection();

	if (sel.rangeCount > 0) {
		range = sel.getRangeAt(0);
		range.deleteContents();
		range.insertNode(node);
		range.collapse(false);
		sel.removeAllRanges();
		sel.addRange(range);
	}
}

function OnTouchStart_OnImage(obj, ev) {
	console.log(arguments.callee.name + "()");
	g_isTouchStartOnImage = true;
}

function OnTouchMove_OnImage(ev) {
	console.log(arguments.callee.name + "()");
	if (!g_isTouchStartOnImage) {
		return;
	}
	g_isTouchMoveOnImage = true;
}

function OnTouchEnd_OnImage(obj, ev) {
	console.log(arguments.callee.name + "()");

	if (g_isTouchStartOnImage && !g_isTouchMoveOnImage) {
		console.log("clicked!");

		ev.preventDefault(); // To prevent loading ime when ime is hidden. (focus is on ewk_view)
		document.body.blur(); // To prevent loading ime when ewk_view get the focus.

		var id = obj.getAttribute("id");
		//SendUserEvent("clickimage://" + id);
	}
	g_isTouchStartOnImage = false;
	g_isTouchMoveOnImage = false;
}

/* Related to the behavior of Image handler */
function RemoveSelectedImage(obj, ev) {
	console.log(arguments.callee.name + "()");

	if (g_whileResizing == true) {
		console.log("return! dragging event occurs!");
		return;
	}
	ev.stopPropagation();

	RemoveImageControlLayer();
	obj.parentNode.removeChild(obj);
	EmitSignal_ResizeEnd();

	if (!g_caretPosition) {
		var scrOfY = parseInt(document.body.scrollTop);
		var scrOfX = parseInt(document.body.scrollLeft);
		g_caretPosition = document.caretRangeFromPoint(parseInt(g_dragData.x) - scrOfX, parseInt(g_dragData.y) - scrOfY);
	}
	var range = document.createRange();
	range.setStart(g_caretPosition.startContainer, g_caretPosition.startOffset);
	range.setEnd(g_caretPosition.startContainer, g_caretPosition.startOffset);
	var sel = window.getSelection();
	sel.removeAllRanges();
	sel.addRange(range);
	g_caretPosition.insertNode(img);
	g_caretPosition = null;
}

function FindParentMessageDiv(node) {
	console.log(arguments.callee.name + "()");

	var newMsg = document.getElementById(G_NAME_NEW_MESSAGE);
	var orgMsg = document.getElementById(G_NAME_ORG_MESSAGE);

	if ((newMsg == null) && (orgMsg == null)) {
		return null;
	}

	console.log("new.offsetLeft: " + newMsg.offsetLeft + ", offsetTop: " + newMsg.offsetTop);
	console.log("org.offsetLeft: " + orgMsg.offsetLeft + ", offsetTop: " + orgMsg.offsetTop);
	console.log("new.scrollTop: " + newMsg.scrollTop + ", scrollLeft: " + newMsg.scrollLeft);
	console.log("org.scrollTop: " + orgMsg.scrollTop + ", scrollLeft: " + orgMsg.scrollLeft);

	var parent = node;
	while (parent.parentNode) {
		if (parent.parentNode == newMsg) {
			parent = newMsg;
			break;
		} else if (parent.parentNode == orgMsg) {
			parent = orgMsg;
			break;
		}
		parent = parent.parentNode;
	}

	return parent;
}

function CreateImageControlLayer(obj) {
	console.log(arguments.callee.name + "()");
	console.log("obj :" + "left:" + obj.x + ", top:" + obj.y + ", height:" + obj.height + ", width:" + obj.width);
	console.log("obj.parent:" + obj.parentNode + ", left:" + obj.offsetParent.offsetLeft + ", top:" + obj.offsetParent.offsetTop);

	var layer = document.createElement("div");
	layer.setAttribute("id", "EmailImageControlLayer");
	layer.setAttribute("isSelected", obj.getAttribute("id"));
	layer.style.cssText = "position:absolute; z-index:11; height:" + obj.height + "px; width:" + obj.width + "px; background-color: rgba(135, 206, 250, 0.4);"; // mean skyblue
	layer.style.borderWidth = G_VAL_LAYER_BORDER + "px " + G_VAL_LAYER_BORDER + "px " + G_VAL_LAYER_BORDER + "px " + G_VAL_LAYER_BORDER + "px";
	layer.style.borderImage = 'url(file:///usr/apps/org.tizen.email/res/images/composer_icon/M02_photo_resize_line.png) 25% 25% 25% 25% stretch';
	//layer.style.outline = "1px solid rgb(0, 156, 255)";

	obj.parentNode.appendChild(layer);

	CreateDotImage(layer, "dot_tl");
	CreateDotImage(layer, "dot_tm");
	CreateDotImage(layer, "dot_tr");
	CreateDotImage(layer, "dot_ml");
	CreateDotImage(layer, "dot_mr");
	CreateDotImage(layer, "dot_bl");
	CreateDotImage(layer, "dot_bm");
	CreateDotImage(layer, "dot_br");

	CreateHiddenControlLayer(obj);
}

function UpdateImageControlLayer() {
	console.log(arguments.callee.name + "()");

	var layer = document.getElementById("EmailImageControlLayer");
	if (layer) {
		var img = document.getElementById(layer.getAttribute("isSelected"));

		// The two logs below are needed. If there aren't the logs, layer is drawn on a wrong place. maybe there's an time issue.
		console.log("img :" + "left:" + img.offsetLeft + ", top:" + img.offsetTop);
		console.log("img :" + "x:" + img.x + ", y:" + img.y);

		var parent = FindParentMessageDiv(layer);
		if (parent) {
			layer.style.top = img.y - Math.floor(G_VAL_LAYER_BORDER) - parent.offsetTop;
			layer.style.left = img.x - Math.floor(G_VAL_LAYER_BORDER) - parent.offsetLeft + parent.scrollLeft;
		} else {
			layer.style.top = img.y - Math.floor(G_VAL_LAYER_BORDER) - document.body.offsetTop;
			layer.style.left = img.x - Math.floor(G_VAL_LAYER_BORDER) - document.body.offsetLeft;
		}
	}

	UpdateDotImage(layer);
	UpdateHiddenControlLayer();
}

function RemoveImageControlLayer() {
	console.log(arguments.callee.name + "()");

	var layer = document.getElementById("EmailImageControlLayer");
	if (layer) {
		var id = layer.getAttribute("isSelected");
		var img = document.getElementById(id);

		img.style.opacity = 1;
		img.removeAttribute("isSelected");

		layer.parentNode.removeChild(layer);
	}
	RemoveHiddenControlLayer();

	g_selectedImg = null;
	g_removeBtn = null;
}

function CreateHiddenControlLayer(obj) {
	console.log(arguments.callee.name + "()");

	var t = obj.y - G_VAL_TOUCH_SPACE;
	var l = obj.x - G_VAL_TOUCH_SPACE;

	var parent = FindParentMessageDiv(obj);
	if (parent != null) {
		t = t - parent.offsetTop;
		l = l - parent.offsetLeft + parent.scrollLeft;
	}

	var h = obj.height + G_VAL_TOUCH_SPACE_DIFF;
	var w = obj.width + G_VAL_TOUCH_SPACE_DIFF;

	var layer = document.createElement("div");
	layer.setAttribute("id", "EmailHiddenControlLayer");
	layer.style.cssText = "position:absolute; z-index:11; top: " + t + "; left: " + l + "; height:" + h + "px; width:" + w + "px;";
	//layer.style.outline = "1px solid rgb(0, 255, 0)";

	obj.parentNode.appendChild(layer);
	var control_layer = document.getElementById("EmailImageControlLayer")

	layer.addEventListener('touchstart', function() { OnTouchStart_OnHiddenControlLayer(this, event) }, false);
	layer.addEventListener('touchmove', function() { OnTouchMove_OnHiddenControlLayer(this, control_layer, event) }, false);
	layer.addEventListener('touchend', function() { OnTouchEnd_OnHiddenControlLayer(control_layer, event) }, false);
	layer.addEventListener('click', function() { OnClick_OnHiddenControlLayer(); }, false);
}

function UpdateHiddenControlLayer() {
	console.log(arguments.callee.name + "()");

	var controlLayer = document.getElementById("EmailImageControlLayer")
	var hiddenLayer = document.getElementById("EmailHiddenControlLayer");
	if (hiddenLayer) {
		var img = document.getElementById(controlLayer.getAttribute("isSelected"));

		// The two logs below are needed. If there aren't the logs, layer is drawn on a wrong place. maybe there's an time issue.
		console.log("img :" + "left:" + img.offsetLeft + ", top:" + img.offsetTop);
		console.log("img :" + "x:" + img.x + ", y:" + img.y);

		var parent = FindParentMessageDiv(controlLayer);
		if (parent) {
			hiddenLayer.style.top = img.y - G_VAL_TOUCH_SPACE - parent.offsetTop;
			hiddenLayer.style.left = img.x - G_VAL_TOUCH_SPACE - parent.offsetLeft + parent.scrollLeft;
		} else {
			hiddenLayer.style.top = img.y - G_VAL_TOUCH_SPACE;
			hiddenLayer.style.left = img.x - G_VAL_TOUCH_SPACE;
		}
	}

	//if ((parseInt(controlLayer.style.width) < G_VAL_REMOVE_BTN_WH) || (parseInt(controlLayer.style.height) < G_VAL_REMOVE_BTN_WH)) {
		//RemoveRemoveImageButton(hiddenLayer);
	//} else {
		if (g_removeBtn == null) {
			CreateRemoveImageButton(hiddenLayer, document.getElementById(controlLayer.getAttribute("isSelected")));
		}
	//}
}

function RemoveHiddenControlLayer() {
	console.log(arguments.callee.name + "()");

	var layer = document.getElementById("EmailHiddenControlLayer");
	if (layer) {
		layer.parentNode.removeChild(layer);
	}
}

function CreateRemoveImageButton(obj, imageObj) {
	console.log(arguments.callee.name + "()");

	var btn = document.createElement("div");
	btn.setAttribute("id", "remove_image_button");
	btn.setAttribute("role", "button");
	btn.setAttribute("aria-label", "Double tap to delete image");
	btn.style.cssText = "position:absolute; z-index:0; top: " + G_VAL_TOUCH_SPACE + "; left: " + G_VAL_TOUCH_SPACE + "; width: " + G_VAL_REMOVE_BTN_WH + "px; height: " + G_VAL_REMOVE_BTN_WH + ";";
	btn.style.backgroundImage = 'url(file:///usr/apps/org.tizen.email/res/images/composer_icon/button_delete.png)';
	btn.style.backgroundRepeat = "no-repeat";
	btn.style.backgroundSize = "100% 100%";
	btn.addEventListener('touchend', function() { RemoveSelectedImage(imageObj, event); }, false);
	obj.appendChild(btn);

	g_removeBtn = btn;
}

function RemoveRemoveImageButton(obj) {
	console.log(arguments.callee.name + "()");

	if (g_removeBtn) {
		obj.removeChild(g_removeBtn);
		g_removeBtn = null;
	}
}

function CreateDotImage(obj, img_id) {
	console.log(arguments.callee.name + "()");

	var dot_image = document.createElement("div");
	dot_image.setAttribute("id", img_id);
	dot_image.style.cssText = "position:absolute; z-index:0; width: " + G_VAL_DOW_W + "px; height: " + G_VAL_DOW_H + "px;";
	dot_image.style.backgroundImage = 'url(file:///usr/apps/org.tizen.email/res/images/composer_icon/M02_photo_resize_dot.png)';
	dot_image.style.backgroundRepeat = "no-repeat";
	dot_image.style.backgroundSize = "100% 100%";
	obj.appendChild(dot_image);
}

function UpdateDotImage(obj) {
	console.log(arguments.callee.name + "()");

	var tl = document.getElementById("dot_tl");
	var tm = document.getElementById("dot_tm");
	var tr = document.getElementById("dot_tr");
	var ml = document.getElementById("dot_ml");
	var mr = document.getElementById("dot_mr");
	var bl = document.getElementById("dot_bl");
	var bm = document.getElementById("dot_bm");
	var br = document.getElementById("dot_br");

	tl.style.top = 0 - (G_VAL_DOW_H/2);
	tl.style.left = 0 - (G_VAL_DOW_W/2);

	tm.style.top = 0 - (G_VAL_DOW_H/2);
	tm.style.left = (parseInt(obj.style.width)/2) - (G_VAL_DOW_W/2) + (1/G_VAL_PIXEL_RATIO);

	tr.style.top = 0 - (G_VAL_DOW_H/2);
	tr.style.left = parseInt(obj.style.width) - (G_VAL_DOW_W/2);

	ml.style.top = (parseInt(obj.style.height)/2) - (G_VAL_DOW_H/2) + (1/G_VAL_PIXEL_RATIO);
	ml.style.left = 0 - (G_VAL_DOW_W/2);

	mr.style.top = (parseInt(obj.style.height)/2) - (G_VAL_DOW_H/2) + (1/G_VAL_PIXEL_RATIO);
	mr.style.left = parseInt(obj.style.width) - (G_VAL_DOW_W/2) + (1/G_VAL_PIXEL_RATIO);

	bl.style.top = parseInt(obj.style.height) - (G_VAL_DOW_H/2) + (1/G_VAL_PIXEL_RATIO);
	bl.style.left = 0 - (G_VAL_DOW_W/2);

	bm.style.top = parseInt(obj.style.height) - (G_VAL_DOW_H/2) + (1/G_VAL_PIXEL_RATIO);
	bm.style.left = (parseInt(obj.style.width)/2) - (G_VAL_DOW_W/2) + (1/G_VAL_PIXEL_RATIO);

	br.style.top = parseInt(obj.style.height) - (G_VAL_DOW_H/2) + (1/G_VAL_PIXEL_RATIO);
	br.style.left = parseInt(obj.style.width) - (G_VAL_DOW_W/2);
}

function OnTouchStart_OnHiddenControlLayer(obj, ev) {
	console.log(arguments.callee.name + "()");

	//ev.preventDefault();
	ev.stopPropagation();
	if (g_dragData == null) {
		var GAP = 30;

		var w = parseInt(obj.style.width);
		var h = parseInt(obj.style.height);

		g_dragData = {
			x: ev.touches[0].pageX,
			y: ev.touches[0].pageY
		};

		//console.log("ev.touches[0].pageX: " + ev.touches[0].pageX + ", ev.touches[0].pageY: " + ev.touches[0].pageY);
		//console.log("obj.offsetLeft: " + obj.offsetLeft + ", offsetTop: " + obj.offsetTop);
		//console.log("w:" + w + ", h:" + h + ", x:" + g_dragData.x + ", y:" + g_dragData.y);

		var offsetLeft = obj.offsetLeft;
		var offsetTop = obj.offsetTop;

		var parent = FindParentMessageDiv(obj);
		if (parent) {
			offsetLeft = offsetLeft + parent.offsetLeft - parent.scrollLeft;
			offsetTop = offsetTop + parent.offsetTop;
		}

		if ((ev.touches[0].pageX >= offsetLeft + w - GAP) && (ev.touches[0].pageY >= offsetTop + h - GAP))
		{
			//console.log("RIGHT_BOTTOM");
			g_dragData.d = Direction.RIGHT_BOTTOM;
		}
		else if ((ev.touches[0].pageX >= offsetLeft + w - GAP) && (ev.touches[0].pageY <= offsetTop + GAP))
		{
			//console.log("RIGHT_TOP");
			g_dragData.d = Direction.RIGHT_TOP;
		}
		else if ((ev.touches[0].pageX <= offsetLeft + GAP) && (ev.touches[0].pageY >= offsetTop + h - GAP))
		{
			//console.log("LEFT_BOTTOM");
			g_dragData.d = Direction.LEFT_BOTTOM;
		}
		else if ((ev.touches[0].pageX <= offsetLeft + GAP) && (ev.touches[0].pageY <= offsetTop + GAP))
		{
			//console.log("LEFT_TOP");
			g_dragData.d = Direction.LEFT_TOP;
		}
		else if (((ev.touches[0].pageX > offsetLeft + GAP) && (ev.touches[0].pageX < offsetLeft + w - GAP)) && (ev.touches[0].pageY >= offsetTop + h - GAP))
		{
			//console.log("BOTTOM");
			g_dragData.d = Direction.BOTTOM;
		}
		else if (((ev.touches[0].pageX > offsetLeft + GAP) && (ev.touches[0].pageX < offsetLeft + w - GAP)) && (ev.touches[0].pageY <= offsetTop + GAP))
		{
			//console.log("TOP");
			g_dragData.d = Direction.TOP;
		}
		else if (((ev.touches[0].pageY > offsetTop + GAP) && (ev.touches[0].pageY < offsetTop + h - GAP)) && (ev.touches[0].pageX >= offsetLeft + w - GAP))
		{
			//console.log("RIGHT");
			g_dragData.d = Direction.RIGHT;
		}
		else if (((ev.touches[0].pageY > offsetTop + GAP) && (ev.touches[0].pageY < offsetTop + h - GAP)) && (ev.touches[0].pageX <= offsetLeft + GAP))
		{
			//console.log("LEFT");
			g_dragData.d = Direction.LEFT;
		}
		else
		{
			//console.log("MOVE");
			g_dragData.d = Direction.MOVE;
		}
	}
}

function OnTouchMove_OnHiddenControlLayer(obj, controlObj, ev) {
	//console.log(arguments.callee.name + "()");

	if (g_dragData != null) {
		if (!g_whileResizing && (g_dragData.d != Direction.MOVE)) {
			EmitSignal_ResizeStart();
		}
		ev.preventDefault();
		g_whileResizing = true;

		var LIMIT = 20 + G_VAL_TOUCH_SPACE_DIFF;
		var w = parseInt(obj.style.width);
		var h = parseInt(obj.style.height);
		var t = parseInt(obj.style.top);
		var l = parseInt(obj.style.left);

		var diff_x = 0;
		var diff_y = 0;

		diff_x = ev.touches[0].pageX - g_dragData.x;
		diff_y = ev.touches[0].pageY - g_dragData.y;
		g_dragData.x = ev.touches[0].pageX;
		g_dragData.y = ev.touches[0].pageY;

		switch (g_dragData.d) {
			case Direction.RIGHT_BOTTOM:
				if ((w + diff_x) > LIMIT) {
					obj.style.width = w + diff_x;
					controlObj.style.width = parseInt(obj.style.width) - G_VAL_TOUCH_SPACE_DIFF;
				}
				if ((h + diff_y) > LIMIT) {
					obj.style.height = h + diff_y;
					controlObj.style.height = parseInt(obj.style.height) - G_VAL_TOUCH_SPACE_DIFF;
				}
				break;

			case Direction.RIGHT_TOP:
				if ((w + diff_x) > LIMIT) {
					obj.style.width = w + diff_x;
					controlObj.style.width = parseInt(obj.style.width) - G_VAL_TOUCH_SPACE_DIFF;
				}
				if ((h - diff_y) > LIMIT) {
					obj.style.top = t + diff_y;
					obj.style.height = h - diff_y;
					controlObj.style.top = parseInt(obj.style.top) + G_VAL_TOUCH_SPACE;
					controlObj.style.height = parseInt(obj.style.height) - G_VAL_TOUCH_SPACE_DIFF;
				}
				break;

			case Direction.LEFT_BOTTOM:
				if ((w - diff_x) > LIMIT) {
					obj.style.left = l + diff_x;
					obj.style.width = w - diff_x;
					controlObj.style.left = parseInt(obj.style.left) + G_VAL_TOUCH_SPACE;
					controlObj.style.width = parseInt(obj.style.width) - G_VAL_TOUCH_SPACE_DIFF;
				}
				if ((h + diff_y) > LIMIT) {
					obj.style.height = h + diff_y;
					controlObj.style.height = parseInt(obj.style.height) - G_VAL_TOUCH_SPACE_DIFF;
				}
				break;

			case Direction.LEFT_TOP:
				if ((w - diff_x) > LIMIT) {
					obj.style.left = l + diff_x;
					obj.style.width = w - diff_x;
					controlObj.style.left = parseInt(obj.style.left) + G_VAL_TOUCH_SPACE;
					controlObj.style.width = parseInt(obj.style.width) - G_VAL_TOUCH_SPACE_DIFF;
				}
				if ((h - diff_y) > LIMIT) {
					obj.style.top = t + diff_y;
					obj.style.height = h - diff_y;
					controlObj.style.top = parseInt(obj.style.top) + G_VAL_TOUCH_SPACE;
					controlObj.style.height = parseInt(obj.style.height) - G_VAL_TOUCH_SPACE_DIFF;
				}
				break;

			case Direction.BOTTOM:
				if ((h + diff_y) > LIMIT) {
					obj.style.height = h + diff_y;
					controlObj.style.height = parseInt(obj.style.height) - G_VAL_TOUCH_SPACE_DIFF;
				}
				break;

			case Direction.RIGHT:
				if ((w + diff_x) > LIMIT) {
					obj.style.width = w + diff_x;
					controlObj.style.width = parseInt(obj.style.width) - G_VAL_TOUCH_SPACE_DIFF;
				}
				break;

			case Direction.LEFT:
				if ((w - diff_x) > LIMIT) {
					obj.style.left = l + diff_x;
					obj.style.width = w - diff_x;
					controlObj.style.left = parseInt(obj.style.left) + G_VAL_TOUCH_SPACE;
					controlObj.style.width = parseInt(obj.style.width) - G_VAL_TOUCH_SPACE_DIFF;
				}
				break;

			case Direction.TOP:
				if ((h - diff_y) > LIMIT) {
					obj.style.top = t + diff_y;
					obj.style.height = h - diff_y;
					controlObj.style.top = parseInt(obj.style.top) + G_VAL_TOUCH_SPACE;
					controlObj.style.height = parseInt(obj.style.height) - G_VAL_TOUCH_SPACE_DIFF;
				}
				break;

			case Direction.MOVE:
				var scrOfY = parseInt(document.body.scrollTop);
				var scrOfX = parseInt(document.body.scrollLeft);
				if (g_caretPosition == null) {
					SendUserEvent("startdrag://");
				}
				g_caretPosition = document.caretRangeFromPoint(parseInt(g_dragData.x) - scrOfX, parseInt(g_dragData.y) - scrOfY);
				if (g_caretPosition) {
					var range = document.createRange();
					range.setStart(g_caretPosition.startContainer, g_caretPosition.startOffset);
					range.setEnd(g_caretPosition.startContainer, g_caretPosition.startOffset);
					var sel = window.getSelection();
					sel.removeAllRanges();
					sel.addRange(range);
				}
				break;

			default:
				//console.log("NEVER ENTER HERE!");
				break;
		}

		if (g_dragData.d != Direction.MOVE) {
			UpdateDotImage(controlObj);
		}
	}
}

function OnTouchEnd_OnHiddenControlLayer(controlObj, ev) {
	console.log(arguments.callee.name + "()");

	var id = controlObj.getAttribute("isSelected");
	var img = document.getElementById(id);

	img.width = parseInt(controlObj.style.width);
	img.height = parseInt(controlObj.style.height);
	if ((g_dragData.d == Direction.MOVE)) {
		SendUserEvent("stopdrag://");
		RemoveImageControlLayer();
		if (!g_caretPosition) {
			var scrOfY = parseInt(document.body.scrollTop);
			var scrOfX = parseInt(document.body.scrollLeft);
			g_caretPosition = document.caretRangeFromPoint(parseInt(g_dragData.x) - scrOfX, parseInt(g_dragData.y) - scrOfY);
		}
		var range = document.createRange();
		range.setStart(g_caretPosition.startContainer, g_caretPosition.startOffset);
		range.setEnd(g_caretPosition.startContainer, g_caretPosition.startOffset);
		var sel = window.getSelection();
		sel.removeAllRanges();
		sel.addRange(range);
		g_caretPosition.insertNode(img);
		g_caretPosition = null;
	} else {
		UpdateImageControlLayer();
		EmitSignal_ResizeEnd();
	}
	g_whileResizing = false;

	if (g_dragData != null)
		g_dragData = null;
}

function OnClick_OnHiddenControlLayer() {
	console.log(arguments.callee.name + "()");
	SendUserEvent("imagecontrollayer://");
}

function EmitSignal_ResizeStart() {
	console.log(arguments.callee.name + "()");
	SendUserEvent("resizestart://");
}

function EmitSignal_ResizeEnd() {
	console.log(arguments.callee.name + "()");
	SendUserEvent("resizeend://");
}

/* Used by c-code */
function InitializeEmailComposer() {
	console.log(arguments.callee.name + "()");

	// Related to "Image control"
	/*var id = 0;
	var images = document.images;
	if (images != null) {
		for (var i = 0; i < images.length; i++) {
			images[i].style.opacity = 1;
			if (images[i].getAttribute('id') == null) {
				images[i].setAttribute('id', id.toString());
				id += 1;
			}
			images[i].addEventListener('touchstart', function() { OnTouchStart_OnImage(this, event); }, false);
			images[i].addEventListener('touchmove', function() { OnTouchMove_OnImage(event); }, false);
			images[i].addEventListener('touchend', function() { OnTouchEnd_OnImage(this, event); }, false);
		}
	}
	*/

	var originalMessageBar = document.getElementById(G_NAME_ORG_MESSAGE_BAR);
	if (originalMessageBar) {
		originalMessageBar.addEventListener('touchstart', function() { OnTouchStart_OriginalMessageBar(event); }, false);
		originalMessageBar.addEventListener('touchmove', function() { OnTouchMove_OriginalMessageBar(event); }, false);
		originalMessageBar.addEventListener('touchend', function() { OnTouchEnd_OriginalMessageBar(event); }, false);
		var originalMessageBarCheckbox = document.getElementById(G_NAME_ORG_MESSAGE_BAR_CHECKBOX);
		if (originalMessageBarCheckbox) {
			originalMessageBarCheckbox.addEventListener('keydown', function() { OnkeyDown_Checkbox(this, event); }, false);
		}

		var newMessage = document.getElementById(G_NAME_NEW_MESSAGE);
		if (newMessage) {
			newMessage.addEventListener('focus', function() { OnFocus_NewMessageDIV(event); }, false);
			newMessage.addEventListener('paste', function() { OnPaste(this, event);}, false);
			newMessage.addEventListener('keydown', function() { OnKeyDown(this, event);}, false);
		}

		var originalMessage = document.getElementById(G_NAME_ORG_MESSAGE);
		if (originalMessage) {
			originalMessage.addEventListener('focus', function() { OnFocus_OrgMessageDIV(event); }, false);
			originalMessage.addEventListener('paste', function() { OnPaste(this, event);}, false);
			originalMessage.addEventListener('keydown', function() { OnKeyDown(this, event);}, false);
		}
	} else {
		document.body.addEventListener('paste', function() { OnPaste(this, event);}, false);
		document.body.addEventListener('keydown', function() { OnKeyDown(this, event);}, false);
	}

	document.body.addEventListener('DOMNodeInserted', function() { OnNodeInserted(this, event);}, false);

	document.addEventListener('selectionchange', function() { OnSelectionChanged(); }, false);
}

function OnSelectionChanged() {

	console.log(arguments.callee.name + "()");

	var res = GetCurrentFontStyleProperties(false);
	if (res != null) {
		SendUserEvent("getchangedtextstyle://" + res);
	}
	NotifyCaretPosChange();
}

function GetCurFontParamsString() {
	var temp = g_curBgColor;
	if (g_curBgColor == "rgba(0, 0, 0, 0)") {
		temp = G_VAL_DEFAULT_BG_COLOR;
	}

	return "font_size=" + g_curFontSize + "&bold=" + g_curBold +
	"&underline=" + g_curUnderline + "&italic=" + g_curItalic +
	"&font_color=" + g_curFontColor + "&bg_color=" + temp +
	"&ordered_list=" +  g_curOrderedList + "&unordered_list=" + g_curUnorderedList;
}

function HexToRgb(hex) {
	var bigint = parseInt(hex, 16);
	var r = (bigint >> 16) & 255;
	var g = (bigint >> 8) & 255;
	var b = bigint & 255;

	return r + "," + g + "," + b;
}

function ExecCommand(type, value) {
	var selectionState = GetSelectionState();
	switch(type){
	case "bold":
		document.execCommand('bold', false, null);
		if (!selectionState) {
			g_curBold = (g_curBold == 1) ? 0 : 1;
		}
		break;
	case "italic":
		document.execCommand('italic', false, null);
		if (!selectionState) {
			g_curItalic = (g_curItalic == 1) ? 0 : 1;
		}
		break;
	case "underline":
		document.execCommand('underline', false, null);
		if (!selectionState) {
			g_curUnderline = (g_curUnderline == 1) ? 0 : 1;
		}
		break;
	case "fontColor":
		document.execCommand('ForeColor', false, value);
		if (!selectionState) {
			g_curFontColor = "rgb(" + HexToRgb(value) + ")";
		}
		break;
	case "backgroudColor":
		document.execCommand('BackColor', false, value);
		if (!selectionState) {
			g_curBgColor = "rgb(" + HexToRgb(value) + ")";
		}
		break;
	case "fontSize":
		document.execCommand('FontSize', false, value);
		if (!selectionState) {
			g_curFontSize = FontSize[value] + "px";
		}
		break;
	case "orderedList":
		document.execCommand('insertOrderedList', false, null);
		if (!selectionState) {
			g_curOrderedList = (g_curOrderedList == 1) ? 0 : 1;
			if (g_curOrderedList == 1 && g_curUnorderedList == 1) {
				g_curUnorderedList = 0;
			}
		}
		break;
	case "unorderedList":
		document.execCommand('insertUnorderedList', false, null);
		if (!selectionState) {
			g_curUnorderedList = (g_curUnorderedList == 1) ? 0 : 1;
			if (g_curUnorderedList == 1 && g_curOrderedList == 1) {
				g_curOrderedList = 0;
			}
		}
		break;
	default:
		return;
	}

	if (selectionState) {
		console.log(arguments.callee.name + "()");
		var font_params = GetCurrentFontStyleProperties(false);
		if (font_params != null) {
			SendUserEvent("getchangedtextstyle://" + font_params);
		}
	} else {
		SendUserEvent("getchangedtextstyle://" + GetCurFontParamsString());
	}
}

function GetCurrentFontStyle() {
	var res = GetCurrentFontStyleProperties(true);
	if (res != null) {
		SendUserEvent("getchangedtextstyle://" + res);
		return 1;
	} else {
		return 0;
	}
}

function CreateImageHandler(inImageId) {
	console.log(arguments.callee.name + "()");

	if (g_selectedImg) {
		RemoveImageControlLayer();
	}

	var images = document.images;
	for (var i = 0; i < images.length; i++) {
		if (images[i].getAttribute("id") == inImageId && !images[i].hasAttribute("isSelected")) {
			console.log("Image selected!");
			g_selectedImg = images[i];
			images[i].setAttribute("isSelected", "true");
			images[i].blur();
			CreateImageControlLayer(images[i]);
			UpdateImageControlLayer();
			break;
		}
	}
}

function ConnectEventListenerFor(image_id) {
	console.log(arguments.callee.name + "()");

	var imageObj = document.getElementById(div_id);
	imageObj.addEventListener('touchstart', function() { OnTouchStart_OnImage(this, event); }, false);
	imageObj.addEventListener('touchmove', function() { OnTouchMove_OnImage(event); }, false);
	imageObj.addEventListener('touchend', function() { OnTouchEnd_OnImage(this, event); }, false);
}

function IsCheckboxChecked() {
	console.log(arguments.callee.name + "()");

	var orgMsgBarCheckbox = document.getElementById(G_NAME_ORG_MESSAGE_BAR_CHECKBOX);

	if (orgMsgBarCheckbox && (orgMsgBarCheckbox.className == G_NAME_CSS_ORG_MESSAGE_BAR_CHECKBOX_CHECKED)) {
		return 'true';
	} else {
		return 'false';
	}
}

function HasOriginalMessageBar() {
	console.log(arguments.callee.name + "()");

	if (document.getElementById(G_NAME_ORG_MESSAGE_BAR) != null) {
		return 'true';
	} else {
		return 'false';
	}
}

function InsertOriginalMessageText(r, g, b, a, text) {
	console.log(arguments.callee.name + "()");

	var newSpan = document.createElement("span");
	newSpan.setAttribute("id", G_NAME_ORG_MESSAGE_BAR_TEXT);
	newSpan.style.cssText = G_VAL_ORG_MESSAGE_BAR_TEXT_ALIGN + " font-size:" + G_VAL_ORG_MESSAGE_BAR_TEXT_SIZE + "px; color: rgb(" + r + "," + g + "," + b + "); opacity=" + a / 255.0 + ";"
	newSpan.innerHTML = text;

	var orgMessageBarDivRow = document.getElementById(G_NAME_ORG_MESSAGE_BAR_ROW);
	orgMessageBarDivRow.insertBefore(newSpan, orgMessageBarDivRow.firstChild);
}

function UpdateOriginalMessageText(text) {
	console.log(arguments.callee.name + "()");
	document.getElementById(G_NAME_ORG_MESSAGE_BAR_TEXT).innerHTML = text;
}

function InsertOriginalMailInfo(mail_info) {
	console.log(arguments.callee.name + "()");

	var orgMessageDiv = document.getElementById(G_NAME_ORG_MESSAGE);
	var mailInfoDiv = document.createElement("div");
	mailInfoDiv.innerHTML = mail_info;
	orgMessageDiv.insertBefore(mailInfoDiv, orgMessageDiv.firstChild);
}

function InsertSignature(signature) {
	console.log(arguments.callee.name + "()");
	var signatureDiv = document.createElement("div");
	signatureDiv.innerHTML = signature;
	document.body.appendChild(signatureDiv);
}

function InsertSignatureTo(div_id, signature) {
	console.log(arguments.callee.name + "()");
	var signatureDiv = document.createElement("div");
	signatureDiv.innerHTML = signature;
	document.getElementById(div_id).appendChild(signatureDiv);
}

function UpdateMinHeightOf(div_id, height) {
	console.log(arguments.callee.name + "()");
	document.getElementById(div_id).style.minHeight = height + "px";
}

function UpdateBGColorTo(div_id, r, g, b, a) {
	console.log(arguments.callee.name + "()");
	var targetDiv = document.getElementById(div_id);
	targetDiv.style.backgroundColor = "rgb(" + r + "," + g + "," + b + ")";
	targetDiv.style.opacity = a / 255.0;
}

function InsertImage(fileUri) {
	console.log(arguments.callee.name + "()");
	var div, img;

	div = document.createElement("div");
	InsertNodeAtCursor(div);

	img = document.createElement("img")
	img.onload = function() {
		var w, h, maxWidth;

		w = img.width;
		h = img.height;
		maxWidth = Math.max(screen.availWidth, screen.availHeight) - 2 * G_VAL_EDITOR_MARGIN_SIZE;

		if (w > maxWidth) {
			h *= maxWidth / w;
			w = maxWidth;
		}

		img.width = w;
		img.height = h;

		div.appendChild(img);
		div.appendChild(document.createElement("br"));
	}
	img.src = fileUri;
	img.style.margin = G_VAL_IMG_STYLE_MARGIN;
}

function GetImageSourcesFromElement(element) {
	console.log(arguments.callee.name + "()");
	var images = element.getElementsByTagName("img");
	var imagesMax = images.length - 1;
	var curImg, curImgSrc, srcs = "", idCounter = 0, re = /;/g;
	for (var i = 0; i <= imagesMax; i++) {
		curImg = images[i];
		curImgSrc = curImg.src;
		if (curImgSrc.startsWith("file://")) {
			curImg.dataset["id"] = ++idCounter;
			srcs += curImgSrc.replace(re, "%3B") + ((i < imagesMax) ? ";" : "");
		}
	}
	return srcs;
}

function GetImageSources() {
	return GetImageSourcesFromElement(document.body);
}

function GetImageSourcesFrom(div_id) {
	return GetImageSourcesFromElement(document.getElementById(div_id));
}

function GetHtmlContents() {
	console.log(arguments.callee.name + "()");
	return document.body.innerHTML;
	//return elements = document.getElementsByTagName("body")[0].innerHTML;
}

function GetHtmlContentsFrom(div_id) {
	console.log(arguments.callee.name + "()");
	return document.getElementById(div_id).innerHTML;
}

function GetComposedHtmlContents(inlineImageSrcs) {
	console.log(arguments.callee.name + "()");

	var srcNewMsg, srcOrgMsg, dstContent, dstBody, tmp;
	var	images, imagesLength, i, curImg, newImgSrc;

	srcNewMsg = document.getElementById(G_NAME_NEW_MESSAGE);
	srcOrgMsg = srcNewMsg && document.getElementById(G_NAME_ORG_MESSAGE);

	dstContent = document.createElement("html");

	if (!srcOrgMsg || (IsCheckboxChecked() != "true")) {
		if (!srcNewMsg) {
			dstBody = document.body.cloneNode(true);
		} else {
			dstBody = srcNewMsg.cloneNode(true);
			dstBody.removeAttribute("id");
			dstBody.removeAttribute("class");
		}
		dstBody.removeAttribute("contenteditable");
	} else {
		dstBody = document.createElement("body");

		tmp = srcNewMsg.cloneNode(true);
		tmp.removeAttribute("id");
		tmp.removeAttribute("class");
		tmp.removeAttribute("contenteditable");
		dstBody.appendChild(tmp);

		tmp = document.createElement("br");
		dstBody.appendChild(tmp);
		dstBody.appendChild(tmp.cloneNode(false));

		tmp = srcOrgMsg.cloneNode(true);
		tmp.removeAttribute("id");
		tmp.removeAttribute("class");
		tmp.removeAttribute("contenteditable");
		dstBody.appendChild(tmp);
	}

	dstContent.appendChild(dstBody);

	if (typeof inlineImageSrcs === "string") {
		try {
			inlineImageSrcs = JSON.parse(inlineImageSrcs);
			if (typeof inlineImageSrcs !== "object") {
				inlineImageSrcs = null;
			}
		} catch (error) {
			console.log(arguments.callee.name + "() " + error);
			inlineImageSrcs = null;
		}
	} else {
		inlineImageSrcs = null;
	}

	images = dstBody.getElementsByTagName("img");
	imagesLength = images.length;
	i = 0;
	while (i < imagesLength) {
		curImg = images[i];
		if (curImg.src.startsWith("file://")) {
			if (inlineImageSrcs) {
				newImgSrc = inlineImageSrcs[curImg.dataset["id"]];
				if (newImgSrc) {
					curImg.src = newImgSrc;
				} else if (g_removeInvalidImages) {
					curImg.parentNode.removeChild(curImg);
					--imagesLength;
					continue;
				}
			}
			curImg.removeAttribute("data-id");
		}
		++i;
	}

	return dstContent.outerHTML;
}

function GetCaretPosition() {
	var range, rects, rect;
	var sel = window.getSelection();
	var x = 0, top = 0, bottom = 0, scrollY = 0, isCollapsed = 1;

	if (sel.rangeCount > 0) {
		isCollapsed = sel.isCollapsed ? 1 : 0;
		range = sel.getRangeAt(0);
		rects = range.getClientRects();
		if (rects.length > 0) {
			if (g_lastRange) {
				if (range.compareBoundaryPoints(Range.START_TO_START, g_lastRange) != 0) {
					g_lastRangeIsForward = false;
				} else if (range.compareBoundaryPoints(Range.END_TO_END, g_lastRange) != 0) {
					g_lastRangeIsForward = true;
				}
			}
			if (g_lastRangeIsForward) {
				rect = rects[rects.length - 1];
				x = rect.right;
				top = rect.top;
				bottom = rect.bottom;
			} else {
				rect = rects[0];
				x = rect.left;
				top = rect.top;
				bottom = rect.bottom;
			}
			g_lastRange = range;
		} else {
			var spanParent,
				span = document.createElement("span");
			span.appendChild(document.createTextNode("\u200b"));
			range.insertNode(span);
			rects = range.getClientRects();
			if (rects.length > 0) {
				rect = rects[0];
				x = rect.left;
				top = rect.top;
				bottom = rect.bottom;
			}
			spanParent = span.parentNode;
			spanParent.removeChild(span);
			spanParent.normalize();
		}
	} else {
		var node = document.activeElement;
		if (node == document.getElementById(G_NAME_ORG_MESSAGE_BAR_CHECKBOX)) {
			rects = node.getClientRects();
			if (rects.length > 0) {
				rect = rects[0];
				x = rect.left;
				top = rect.top;
				bottom = rect.bottom;
			}
		}
	}

	if (top == bottom) {
		return null;
	}

	scrollY = window.scrollY;
	top += scrollY;
	bottom += scrollY;

	return new CaretPos(x * G_VAL_PIXEL_RATIO,
						top * G_VAL_PIXEL_RATIO,
						bottom * G_VAL_PIXEL_RATIO,
						isCollapsed);
}

function RestoreCurrentSelection() {
	console.log(arguments.callee.name + "()");

	if (g_selectionRange == null) {
		return;
	}

	var selection = window.getSelection();
	if (selection.rangeCount > 0) {
		if (selection.rangeCount == 1) {
			var range = selection.getRangeAt(0);
			var startPointCompareRes = range.compareBoundaryPoints(Range.START_TO_START, g_selectionRange);
			var endPointCompareRes = range.compareBoundaryPoints(Range.END_TO_END, g_selectionRange);

			if (startPointCompareRes == 0 && endPointCompareRes == 0) {
				return;
			}
		}
		selection.removeAllRanges();
	}
	selection.addRange(g_selectionRange);
}

function BackupCurrentSelection() {
	var selection = window.getSelection();
	if (selection.rangeCount) {
		g_selectionRange = selection.getRangeAt(0);
	}
}

function GetSelectionState() {
	BackupCurrentSelection();
	if (g_selectionRange != null && g_selectionRange.toString() != "") {
		return true;
	}
	return false;
}

function GetComputedStyle(el) {
	return window.getComputedStyle(el, null);
}

function GetComputedStyleProperty(el, propName) {
	return window.getComputedStyle(el, null)[propName];
}

function GetCurrentFontStyleProperties(force) {
	var nodeArray = getSelectedNodes();

	if (nodeArray.length == 0) {
		return null;
	}

	var containerEl = null;
	var newFontSize = "", newBold = 1, newUnderline = 1, newItalic = 1, newOrderedList = 1, newUnorderedList = 1;
	var newFontColor = "", newBgColor = "";

	if (nodeArray.length == 1 && nodeArray[0].nodeType != G_VAL_TEXT_NODE_TYPE) {

		newBold = document.queryCommandState('bold') ? 1 : 0;
		newUnderline = document.queryCommandState('underline') ? 1 : 0;
		newItalic = document.queryCommandState('italic') ? 1 : 0;
		newOrderedList = document.queryCommandState('insertOrderedList') ? 1 : 0;
		newUnorderedList = document.queryCommandState('insertUnorderedList') ? 1 : 0;
		newFontColor = document.queryCommandValue('forecolor');
		newBgColor = document.queryCommandValue('backcolor');

		var tempSize = document.queryCommandValue('FontSize');
		if (tempSize == 0) {
			newFontSize = G_VAL_DEFAULT_FONT_SIZE;
		} else {
			newFontSize = FontSize[tempSize] + "px";
		}
	} else {
		for(var i in nodeArray) {
			containerEl = nodeArray[i];
			if (containerEl.nodeType == G_VAL_TEXT_NODE_TYPE) {
				containerEl = containerEl.parentNode;
			}

			var iFontSize = "", iBold = 0, iUnderline = 0, iItalic = 0, iOrderedList = 0, iUnorderedList = 0;
			var iFontColor = "", iBgColor = "";

			var computedStyle = GetComputedStyle(containerEl);
			iFontSize = computedStyle["fontSize"];
			iBold = (computedStyle["font-weight"] == "bold") ? 1 : 0;
			iItalic = (computedStyle["fontStyle"] == "italic") ? 1 : 0;
			iFontColor = computedStyle["color"];
			iBgColor = computedStyle["backgroundColor"];

			while (containerEl.nodeName.toLowerCase() != "body") {
				switch (containerEl.nodeName.toLowerCase())
				{
				case "em":
					iItalic = 1;
					break;
				case "strong":
					iBold = 1;
					break;
				case "ol":
					if (iOrderedList == 0 && iUnorderedList == 0) {
						iOrderedList = 1;
						iUnorderedList = 0;
					}
					break;
				case "ul":
					if (iOrderedList == 0 && iUnorderedList == 0) {
						iOrderedList = 0;
						iUnorderedList = 1;
					}
					break;
				default:
					break;
				}

				if (iBgColor == "rgba(0, 0, 0, 0)") {
					iBgColor = GetComputedStyleProperty(containerEl, "backgroundColor");
				}
				if (iUnderline == 0) {
					iUnderline = (GetComputedStyleProperty(containerEl, "text-decoration") == "underline" ) ? 1 : 0;
				}
				containerEl = containerEl.parentNode;
			}

			newBold = newBold & iBold;
			newUnderline = newUnderline & iUnderline;
			newItalic = newItalic & iItalic;
			newOrderedList = newOrderedList & iOrderedList;
			newUnorderedList = newUnorderedList & iUnorderedList;

			if (newFontSize == "") {
				newFontSize = iFontSize;
			} else if (newFontSize != iFontSize) {
				newFontSize = G_VAL_DEFAULT_FONT_SIZE;
			}

			if (newFontColor == "") {
				newFontColor = iFontColor;
			} else if (newFontColor != iFontColor) {
				newFontColor = G_VAL_DEFAULT_FONT_COLOR;
			}

			if (newBgColor == "") {
				newBgColor = iBgColor;
			} else if (newBgColor != iBgColor) {
				newBgColor = G_VAL_DEFAULT_BG_COLOR;
			}
		}
	}

	if ((force == false &&
		g_curFontSize == newFontSize && g_curBold == newBold &&
		g_curUnderline == newUnderline && g_curItalic == newItalic &&
		g_curOrderedList == newOrderedList && g_curUnorderedList == newUnorderedList &&
		g_curFontColor == newFontColor && g_curBgColor == newBgColor) || newFontSize == 0) {
		return null;
	} else {
		g_curFontSize = newFontSize;
		g_curBold = newBold;
		g_curUnderline = newUnderline;
		g_curItalic = newItalic;
		g_curOrderedList = newOrderedList;
		g_curUnorderedList = newUnorderedList;
		g_curFontColor = newFontColor;
		g_curBgColor = newBgColor;
		return GetCurFontParamsString();
	}
}

function nextNode(node) {
	if (node.hasChildNodes()) {
		return node.firstChild;
	} else {
		while (node && !node.nextSibling) {
			node = node.parentNode;
		}
		if (!node) {
			return null;
		}
		return node.nextSibling;
	}
}

function getRangeSelectedNodes(range) {
	var node = range.startContainer;
	var endNode = range.endContainer;
	var rangeNodes = [];

	while (node) {
		if (node.nodeType == G_VAL_TEXT_NODE_TYPE) {
			rangeNodes.push(node);
		}
		if (node == endNode) {
			break;
		}
		node = nextNode(node);
	}

	if (rangeNodes.length == 0) {
		rangeNodes.push(range.commonAncestorContainer);
	}

	return rangeNodes;
}

function getSelectedNodes() {
	if (window.getSelection) {
		var sel = window.getSelection();
		if (sel.rangeCount > 0) {
			return getRangeSelectedNodes(sel.getRangeAt(0));
		}
	}
	return [];
}
