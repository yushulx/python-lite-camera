import litecam
from dynamsoft_capture_vision_bundle import *
import queue


class FrameFetcher(ImageSourceAdapter):
    def has_next_image_to_fetch(self) -> bool:
        return True

    def add_frame(self, imageData):
        self.add_image_to_buffer(imageData)


class MyCapturedResultReceiver(CapturedResultReceiver):
    def __init__(self, result_queue):
        super().__init__()
        self.result_queue = result_queue

    def on_captured_result_received(self, captured_result):
        self.result_queue.put(captured_result)


if __name__ == '__main__':
    errorCode, errorMsg = LicenseManager.init_license(
        "LICENSE-KEY")
    if errorCode != EnumErrorCode.EC_OK and errorCode != EnumErrorCode.EC_LICENSE_CACHE_USED:
        print("License initialization failed: ErrorCode:",
              errorCode, ", ErrorString:", errorMsg)
    else:
        camera = litecam.PyCamera()
        if camera.open(0):

            cvr = CaptureVisionRouter()
            fetcher = FrameFetcher()
            cvr.set_input(fetcher)

            result_queue = queue.Queue()

            receiver = MyCapturedResultReceiver(result_queue)
            cvr.add_result_receiver(receiver)

            errorCode, errorMsg = cvr.start_capturing(
                EnumPresetTemplate.PT_READ_BARCODES.value)

            if errorCode != EnumErrorCode.EC_OK:
                print("error:", errorMsg)

            window = litecam.PyWindow(
                camera.getWidth(), camera.getHeight(), "Camera Stream")

            while window.waitKey('q'):
                frame = camera.captureFrame()
                if frame is not None:
                    width = frame[0]
                    height = frame[1]
                    size = frame[2]
                    data = frame[3]
                    window.showFrame(width, height, data)

                    imagedata = ImageData(
                        bytes(data), width, height, width * 3, EnumImagePixelFormat.IPF_RGB_888)
                    fetcher.add_frame(imagedata)

                    if not result_queue.empty():
                        captured_result = result_queue.get_nowait()

                        items = captured_result.get_items()
                        for item in items:

                            if item.get_type() == EnumCapturedResultItemType.CRIT_BARCODE:
                                text = item.get_text()
                                location = item.get_location()
                                x1 = location.points[0].x
                                y1 = location.points[0].y
                                x2 = location.points[1].x
                                y2 = location.points[1].y
                                x3 = location.points[2].x
                                y3 = location.points[2].y
                                x4 = location.points[3].x
                                y4 = location.points[3].y

                                contour_points = [
                                    (x1, y1), (x2, y2), (x3, y3), (x4, y4)]
                                window.drawContour(contour_points)

                                window.drawText(text, x1, y1, 24, (255, 0, 0))

                                del location

            camera.release()

            cvr.stop_capturing()
