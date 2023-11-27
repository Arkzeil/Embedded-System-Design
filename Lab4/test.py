import cv2
import numpy as np

# Read an image from file
input_image = cv2.imread("picture.png")

if input_image is None:
    print("Error loading the image.")
    exit()

# Set the shift amount (positive for left shift)
shift_amount = -300
shifted_image = input_image
# Create an output image with additional border space
for i in range(3):
    if shift_amount >= 0:
        shifted_image = cv2.copyMakeBorder(shifted_image, 0, 0, shift_amount, 0, cv2.BORDER_WRAP)
        shifted_image = shifted_image[:, :input_image.shape[1]]
    else:
        shifted_image = cv2.copyMakeBorder(shifted_image, 0, 0, 0, -shift_amount, cv2.BORDER_WRAP)
        shifted_image = shifted_image[:, -input_image.shape[1]:]
    cv2.imshow("Shifted Image", shifted_image)
    cv2.waitKey(0)
    shift_amount += 500
# Crop the result to keep the original image size
# shifted_image = shifted_image[:input_image.shape[0], :input_image.shape[1]]
height, width = shifted_image.shape[:2]
print(width, height)
# Display the original and shifted images
cv2.imshow("Original Image", input_image)
#cv2.imshow("Shifted Image", shifted_image)
cv2.waitKey(0)
cv2.destroyAllWindows()
