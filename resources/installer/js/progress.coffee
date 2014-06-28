#Copyright (c) 2011 ~ 2013 Deepin, Inc.
#              2011 ~ 2013 yilang
#
#Author:      LongWei <yilang2007lw@gmail.com>
#Maintainer:  LongWei <yilang2007lw@gmail.com>
#
#This program is free software; you can redistribute it and/or modify
#it under the terms of the GNU General Public License as published by
#the Free Software Foundation; either version 3 of the License, or
#(at your option) any later version.
#
#This program is distributed in the hope that it will be useful,
#but WITHOUT ANY WARRANTY; without even the implied warranty of
#MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#GNU General Public License for more details.
#
#You should have received a copy of the GNU General Public License
#along with this program; if not, see <http://www.gnu.org/licenses/>.


class ReportDialog extends Dialog
    constructor: (@id) ->
        super(@id, false, @cb)
        @add_css_class("DialogCommon")
        @title_txt.innerText = _("Error Report")
        @report_tips = create_element("p", "", @content)
        @report_tips.innerText = _("System installation failed. Please send the log to Deepin Team.")

    cb: ->
        echo "report dialog cb"
        DCore.Installer.finish_install()

apply_progress_flash = (el, time)->
    apply_animation(el, "progressflash", "#{time}s", "cubic-bezier(0, 0, 0.35, -1)")

__ppt_in_switch = false
__ppt_switch_id = -1
__install_failed = false

class Progress extends Page
    constructor: (@id)->
        super
        @titleimg = create_img("", "images/progress_part.png", @titleprogress)

        @loading = create_element("div", "Loading", @element)
        @loading_tips = create_element("div", "LoadingTxt", @loading)
        @loading_tips.innerText = _("Prepare for Installation")
        @rotate = create_element("div", "Rotate", @loading)
        @deg = 0
        setInterval(=>
            @update_rotate()
        , 30)

        @progress_container = create_element("div", "ProgressContainer", @element)
        @progress_container.style.display = "block"
        @progressbar = create_element("div", "ProgressBar", @progress_container)
        @light = create_element("div", "ProgressLight", @progress_container)
        @light.style.webkitAnimationName = "progressflash"
        @light.style.webkitAnimationDuration = "5s"
        @light.style.webkitAnimationIterationCount = 1000
        @light.style.webkitAnimationTimingFunction = "cubic-bezier(0, 0, 0.35, -1)"

        @ppt = create_element("iframe","ppt_iframe",@element)
        if document.body.lang is "zh"
            @ppt.src = "ppt/slideshow2014/index.html"
        else
            @ppt.src = "ppt/slideshow2014/index_en.html"

        @ticker = 0
        @tu = 180
        @display_progress = false
        setTimeout(=>
            if @display_progress == false
                @display_progress = true
                @start_progress()
        , 2000)

    update_rotate: ->
        if @deg > 360
            @deg = 0
        @rotate.style.webkitTransform = "rotate(#{@deg}deg)"
        @deg += 12

    start_progress: ->
        @titleimg.setAttribute("src", "images/progress_extract.png")
        setTimeout(=>
            @loading.style.display = "none"
            echo "@ppt show"
            @ppt.style.display = "block"
        ,1000)
        apply_animation(@loading, "loadingout", "1s", "linear")
        apply_animation(@progress_container, "pptin", "2s", "linear")
        apply_animation(@ppt, "pptin", "2s", "linear")
        @progress_container.style.display = "block"

    update_progress: (progress) ->
        @progressbar.style.width = progress

    show_report: ->
        __install_failed = true
        __selected_stage = "terminate"
        finish_page = new Finish("finish", false)
        pc.add_page(finish_page)
        pc.remove_page(progress_page)

DCore.signal_connect("install_progress", (per)->
    if per >= 100
        finish_page = new Finish("finish", true)
        pc.remove_page(progress_page)
        pc.add_page(finish_page)
    progress_page?.update_progress("#{per}%")
)
