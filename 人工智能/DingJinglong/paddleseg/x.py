from paddlex import create_model
model = create_model(model_name="Deeplabv3_Plus-R101", model_dir="D:\\code\\models\\paddleseg_1\\models\\Deeplabv3_Plus-R101_Pretrained")
output = model.predict(input="D:\\pic\\b1.jpg", batch_size=1)
for res in output:
    res.print()
    res.save_to_img("D:\\model_output")
    res.save_to_json("D:\\model_output")